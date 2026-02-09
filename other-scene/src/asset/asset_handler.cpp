/**
 * \file asset/asset_handler.cpp
 **/
#include "asset/asset_handler.hpp"

#include <ranges>

#include <asio/asio.hpp>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/subsystem.hpp"
#include "file/filesystem.hpp"

#include "asio/asio/associated_executor.hpp"

namespace {

  void register_file_in_mount(
    other::ref<other::directory> mount,
    const std::string& mount_name, const other::filepath& abs_asset_path,
    const std::string& relative_path
  ) {
    auto components = other::directory::split_path(relative_path);
    if (components.empty()) {
      return;
    }

    std::string file_name = components.back();
    components.pop_back();

    other::ref<other::directory> current = mount;
    other::filepath current_path = mount->absolute_path();
    for (const auto& comp : components) {
      current_path /= comp;
      auto child = current->get_child_directory(comp);
      if (child == nullptr) {
        child = current->add_child_directory(comp, current_path);
      }
      current = child;
    }

    if (current->has_file(file_name)) {
      return;
    }

    auto local = other::make_ref<other::local_file>(abs_asset_path);
    local->parent = current.raw_ptr();
    current->add_file(local);

    CORE_LOG_DEBUG("Registered asset in filesystem: {} (mount: {})", abs_asset_path.string(), mount_name);
  }

}  // anonymous namespace

namespace other {

  void asset_handler::purge_stores() {
    while (!pending_unloads.empty()) {
      natural_t asset_id = pending_unloads.front();
      pending_unloads.pop();

      auto it = loaded_assets.find(asset_id);
      if (it != loaded_assets.end()) {
        loaded_assets.erase(it);
        CORE_LOG_DEBUG("Purged asset from store (ID: {})", asset_id);
      }
    }
  }

  void asset_handler::update_pipelines() {
    if (asset_pipelines.empty()) {
      return;
    }

    for (auto& pl : asset_pipelines) {
      pl.pipeline->poll();
    }
  }

  natural_t asset_handler::load_asset(const filepath& file_path, load_completion_callback on_complete) {
    natural_t asset_id = get_next_asset_id();

    auto* fs = subsystem<file_system>::get();
    bool exists = false;

    if (fs != nullptr) {
      std::string engine_path;
      for (const auto& mount_name : fs->mounted_names()) {
        auto mount = fs->get_mount(mount_name);
        if (mount == nullptr) {
          continue;
        }
        for (const auto& file : mount->files()) {
          if (file->absolute_path() == file_path) {
            exists = true;
            break;
          }
        }
        if (exists) {
          break;
        }
      }
    }

    if (!exists) {
      exists = std::filesystem::exists(file_path);
    }

    if (!exists) {
      CORE_LOG_ERROR("Asset file does not exist: {}", file_path.string());
      return 0;
    }

    if (auto itr = std::ranges::find_if(loaded_assets, [&file_path](const auto& pair) { return pair.second.path_hash == FNV(file_path.string()); });
        itr != loaded_assets.end()) {
      CORE_LOG_WARN("Attempting to reload asset: {}", file_path.string());
      return itr->second.id;
    }

    if (auto itr = std::ranges::find_if(asset_pipelines, [&file_path](const auto& pair) { return pair.loading_asset.path_hash == FNV(file_path.string()); });
        itr != asset_pipelines.end()) {
      CORE_LOG_WARN("Attempting to reload asset: {}", file_path.string());
      return itr->loading_asset.id;
    }

    std::string extension = file_path.extension().string();
    asset::type asset_type = asset::get_type_from_extension(extension);

    if (asset_type == asset::type::EMPTY) {
      CORE_LOG_ERROR("Unsupported asset file extension: {}", extension);
      return 0;
    }

    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{ .pipeline = asset_pipeline::get_asset_pipeline(asset_type, this), .loading_asset = asset{ asset_type, asset_id, FNV(file_path.string()), file_path }, .on_complete = on_complete });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(asset_id, asset_state_machine{});
    if (!state_inserted) {
      CORE_LOG_ERROR("Failed to create asset state machine for asset: {}", file_path.string());
      asset_pipelines.erase(it);
      return 0;
    }
    CORE_LOG_DEBUG("Beginning load for asset ID: {} (Type: {}, Path: {})", asset_id, asset_type, file_path.string());

    asset* loading_asset = &it->loading_asset;
    loading_asset->path_hash = FNV(file_path.string());
    state_it->second.handle_event(asset_event::LOAD_REQUESTED, loading_asset);

    CORE_LOG_TRACE("Executing load operation for asset ID: {}", loading_asset->id);
    auto itr = std::ranges::find_if(asset_pipelines, [id = loading_asset->id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(itr != asset_pipelines.end(), "Loading asset not found in asset pipelines");

    itr->pipeline->start_load(
      thread_pool, &itr->loading_asset,
      [this, id = itr->loading_asset.id]() {
        auto it = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
        OTHER_ASSERT(it != asset_pipelines.end(), "Loaded asset not found in asset pipelines");
        CORE_LOG_DEBUG("Asset load completion handler triggered for asset ID: {}", id);
        if (it->on_complete) {
          it->on_complete(&it->loading_asset);
        }
        on_asset_loaded(&it->loading_asset);
      },
      [this, id = itr->loading_asset.id](const std::string& error_msg) {
        auto it = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
        OTHER_ASSERT(it != asset_pipelines.end(), "Failed asset not found in asset pipelines");
        CORE_LOG_DEBUG("Asset load failure handler triggered for asset ID: {}", id);
        if (it->on_complete) {
          it->on_complete(&it->loading_asset);
        }
        on_asset_load_failed(&it->loading_asset, error_msg);
      }
    );

    return asset_id;
  }

  void asset_handler::unload_asset(natural_t asset_id) {
    auto state_itr = asset_states.find(asset_id);
    if (state_itr == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return;
    }

    if (state_itr->second.get_current_state() == asset_state::UNLOADING) {
      return;
    }

    if (state_itr->second.get_current_state() == asset_state::LOADING) {
      pending_unloads.push(asset_id);
      return;
    }
    CORE_LOG_DEBUG("Unloading asset ID: {}", asset_id);

    state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    auto it = loaded_assets.find(asset_id);
    OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);

    asset* loaded_asset = &it->second;

    asset::type asset_type = loaded_asset->asset_type;
    auto pl_itr = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(asset_type, this),
                                                                  .loading_asset = std::move(*loaded_asset),
                                                                });
    OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    CORE_LOG_TRACE("Executing unload operation for asset ID: {}", loaded_asset->id);
    auto itr = std::ranges::find_if(asset_pipelines, [id = loaded_asset->id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(itr != asset_pipelines.end(), "Unloading asset not found in asset pipelines");

    itr->pipeline->start_unload(
      thread_pool, &itr->loading_asset,
      [this, id = itr->loading_asset.id]() {
        auto it = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
        OTHER_ASSERT(it != asset_pipelines.end(), "Unloaded asset not found in asset pipelines");
        on_asset_unloaded(&it->loading_asset);
        it->pipeline = nullptr;
        asset_pipelines.erase(it);
      },
      [this, id = itr->loading_asset.id](const std::string& error_msg) {
        auto it = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
        OTHER_ASSERT(it != asset_pipelines.end(), "Failed asset not found in asset pipelines");
        on_asset_unload_failed(&it->loading_asset, error_msg);
        it->pipeline = nullptr;
        asset_pipelines.erase(it);
      }
    );
  }

  asset_state asset_handler::get_asset_state(natural_t asset_id) const {
    auto it = asset_states.find(asset_id);
    if (it != asset_states.end()) {
      return it->second.get_current_state();
    }
    return asset_state::ERROR_STATE;
  }

  natural_t asset_handler::get_asset_hash(natural_t asset_id) const {
    auto it = loaded_assets.find(asset_id);
    if (it != loaded_assets.end()) {
      return it->second.path_hash;
    }
    return 0;
  }

  void asset_handler::on_asset_loaded(asset* asset_ptr) {
    CORE_LOG_DEBUG("Asset loaded successfully (ID: {})", asset_ptr->id);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [asset_ptr](const auto& a) { return a.loading_asset.id == asset_ptr->id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Loaded asset not found in loading assets");

    auto itr = loaded_assets.emplace(asset_ptr->id, std::move(pending_itr->loading_asset));
    OTHER_ASSERT(itr.second, "Failed to insert loaded asset into loaded assets map");

    pending_itr->pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    register_asset_in_filesystem(&itr.first->second);

    state_itr->second.handle_event(asset_event::LOAD_COMPLETED);
  }

  void asset_handler::on_asset_load_failed(asset* asset_ptr, const std::string& error_message) {
    CORE_LOG_ERROR("Failed to load asset (ID: {}): {}", asset_ptr->id, error_message);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::ERROR_EVENT);
  }

  void asset_handler::on_asset_unloaded(asset* asset_ptr) {
    CORE_LOG_DEBUG("Asset unloaded successfully (ID: {})", asset_ptr->id);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::UNLOAD_COMPLETED);

    pending_unloads.push(asset_ptr->id);
  }

  void asset_handler::on_asset_unload_failed(asset* asset_ptr, const std::string& error_message) {
    CORE_LOG_ERROR("Failed to unload asset (ID: {}): {}", asset_ptr->id, error_message);

    auto state_itr = asset_states.find(asset_ptr->id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", asset_ptr->id);

    state_itr->second.handle_event(asset_event::ERROR_EVENT);
  }

  natural_t asset_handler::load_asset(const std::string_view engine_path, load_completion_callback on_complete) {
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      CORE_LOG_ERROR("File system subsystem not available for engine path: {}", engine_path);
      return 0;
    }

    if (!fs->file_exists(engine_path)) {
      CORE_LOG_ERROR("Asset not found in file system: {}", engine_path);
      return 0;
    }

    auto file = fs->open(engine_path);
    if (file == nullptr) {
      CORE_LOG_ERROR("Failed to open file from file system: {}", engine_path);
      return 0;
    }

    return load_asset(file->absolute_path(), std::move(on_complete));
  }

  void asset_handler::register_asset_in_filesystem(const asset* asset_ptr) {
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      return;
    }

    const filepath& asset_path = asset_ptr->path;
    if (asset_path.empty()) {
      return;
    }

    CORE_LOG_DEBUG("Registering asset in filesystem: {}", asset_path.string());
    // filepath abs_asset_path = std::filesystem::absolute(asset_path);
    std::string asset_path_str = asset_path.string();

    for (const auto& mount_name : fs->mounted_names()) {
      auto mount = fs->get_mount(mount_name);
      if (mount == nullptr || mount->absolute_path().empty()) {
        continue;
      }

      std::string mount_path_str = mount->absolute_path().string();
      if (!asset_path_str.starts_with(mount_path_str)) {
        continue;
      }

      std::string relative = asset_path_str.substr(mount_path_str.size());
      if (!relative.empty() && (relative.front() == '/' || relative.front() == '\\')) {
        relative = relative.substr(1);
      }

      register_file_in_mount(mount, mount_name, asset_path, relative);
      return;
    }

    auto mount = fs->get_mount(default_mount);
    if (mount == nullptr) {
      mount = fs->mount_virtual(default_mount);
    }

    if (mount == nullptr) {
      CORE_LOG_ERROR("Failed to get or create default mount '{}' for asset: {}", default_mount, asset_path.string());
      return;
    }

    std::string relative = asset_path.parent_path().filename().string() + "/" + asset_path.filename().string();
    register_file_in_mount(mount, default_mount, asset_path, relative);
  }

  asset_state asset_handler::get_asset_state_by_path_hash(natural_t path_hash) const {
    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == path_hash) {
        return get_asset_state(id);
      }
    }

    for (const auto& ctx : asset_pipelines) {
      if (ctx.loading_asset.path_hash == path_hash) {
        return get_asset_state(ctx.loading_asset.id);
      }
    }

    return asset_state::UNLOADED;
  }

  const asset* asset_handler::get_loaded_asset(natural_t asset_id) const {
    auto it = loaded_assets.find(asset_id);
    if (it != loaded_assets.end()) {
      return &it->second;
    }
    return nullptr;
  }

  std::vector<natural_t> asset_handler::get_all_tracked_ids() const {
    std::vector<natural_t> ids;
    ids.reserve(loaded_assets.size() + asset_pipelines.size());

    for (const auto& [id, a] : loaded_assets) {
      ids.push_back(id);
    }

    for (const auto& ctx : asset_pipelines) {
      ids.push_back(ctx.loading_asset.id);
    }

    return ids;
  }

}  // namespace other