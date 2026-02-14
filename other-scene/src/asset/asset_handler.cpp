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

namespace other {

  std::vector<asset::type> asset_handler::get_convertible_asset_types(asset::type requested_type) {
    std::vector<asset::type> out_acceptable_types;
    switch (requested_type) {
      case asset::type::MODEL:
      case asset::type::MODEL_SOURCE:
        return { asset::type::MODEL, asset::type::MODEL_SOURCE };

      case asset::type::SCRIPT:
      case asset::type::SCRIPT_SOURCE:
        return { asset::type::SCRIPT, asset::type::SCRIPT_SOURCE };
      default: break;
    }
    return out_acceptable_types;
  }

  void asset_handler::purge_stores() {
    while (!pending_unloads.empty()) {
      natural_t asset_id = pending_unloads.front();
      pending_unloads.pop();

      if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
        loaded_assets.erase(it);
      }

      if (auto it = unloaded_assets.find(asset_id); it != unloaded_assets.end()) {
        unloaded_assets.erase(it);
      }

      if (auto it = asset_states.find(asset_id); it != asset_states.end()) {
        asset_states.erase(it);
      }
    }

    /// \todo unload all assets, some might need cleanup that we currently ignore
    // for (auto it = loaded_assets.begin(); it != loaded_assets.end();) {
    //   /// add unloading pipelines for each asset to be unloaded
    //   natural_t asset_id = it->first;
    //   auto state_itr = asset_states.find(asset_id);
    //   if (state_itr != asset_states.end() && state_itr->second.get_current_state() != asset_state::UNLOADING) {
    //     CORE_LOG_DEBUG("Purging asset ID: {}", asset_id);

    //     state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    //     asset* loaded_asset = &it->second;

    //     asset::type asset_type = loaded_asset->asset_type;
    //     auto pl_itr = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
    //                                                                   .pipeline = asset_pipeline::get_asset_pipeline(asset_type, this),
    //                                                                   .loading_asset = std::move(*loaded_asset),
    //                                                                 });
    //     OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    //     CORE_LOG_TRACE("Executing unload operation for asset ID: {}", loaded_asset->id);
    //     auto itr = std::ranges::find_if(asset_pipelines, [id = loaded_asset->id](const auto& a) { return a.loading_asset.id == id; });
    //     OTHER_ASSERT(itr != asset_pipelines.end(), "Unloading asset not found in asset pipelines");
    //     itr->pipeline->start_unload(
    //       thread_pool, &itr->loading_asset,
    //       [this, id = itr->loading_asset.id](asset* asset_ptr) {
    //         OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload success callback");
    //         successful_pipelines.push(id);
    //       },
    //       [this, id = itr->loading_asset.id](asset* asset_ptr, const std::string& error_msg) {
    //         OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload failure callback");
    //         failed_pipelines.push(id);
    //       }
    //     );
    //   }
    // }
  }

  void asset_handler::update_pipelines() {
    if (asset_pipelines.empty()) {
      return;
    }

    for (auto& pl : asset_pipelines) {
      pl.pipeline->poll();
    }

    while (!successful_pipelines.empty()) {
      natural_t id = successful_pipelines.front();
      successful_pipelines.pop();

      switch (get_asset_state(id)) {
        case asset_state::LOADING: on_asset_loaded(id); break;
        case asset_state::UNLOADING: on_asset_unloaded(id); break;
        default:
          OTHER_ASSERT(false, "Asset ID {} in unexpected state after successful pipeline completion", id);
      }
    }

    while (!failed_pipelines.empty()) {
      natural_t id = failed_pipelines.front();
      failed_pipelines.pop();

      switch (get_asset_state(id)) {
        case asset_state::LOADING: on_asset_load_failed(id); break;
        case asset_state::UNLOADING: on_asset_unload_failed(id); break;
        default:
          OTHER_ASSERT(false, "Asset ID {} in unexpected state after failed pipeline completion", id);
      }
    }
  }

  natural_t asset_handler::load_asset(const filepath& file_path, load_completion_callback on_complete) {
    natural_t asset_id = get_next_asset_id();

    bool exists = std::filesystem::exists(file_path);
    if (!exists) {
      CORE_LOG_ERROR("Asset file does not exist: {}", file_path.string());
      return 0;
    }

    auto absolute_path = std::filesystem::absolute(file_path);
    natural_t hash = FNV(absolute_path.string());
    if (auto itr = std::ranges::find_if(loaded_assets, [hash](const auto& pair) { return pair.second.path_hash == hash; });
        itr != loaded_assets.end()) {
      return itr->second.id;
    }

    std::string extension = file_path.extension().string();
    asset::type asset_type = asset::get_type_from_extension(extension);
    if (asset_type == asset::type::EMPTY) {
      CORE_LOG_ERROR("Unsupported asset file extension: {}", extension);
      return 0;
    }

    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                              .pipeline = asset_pipeline::get_asset_pipeline(asset_type, this),
                                                              .loading_asset = asset{
                                                                .asset_type = asset_type,
                                                                .id = asset_id,
                                                                .path_hash = hash,
                                                                .load_path = file_path,
                                                                .virtual_path = std::format("{}/{}", default_mount, file_path.string()),
                                                                .absolute_path = std::filesystem::absolute(absolute_path),
                                                              },
                                                              .on_complete = on_complete,
                                                            });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(asset_id, asset_state_machine{});
    if (!state_inserted) {
      CORE_LOG_ERROR("Failed to create asset state machine for asset: {}", file_path.string());
      asset_pipelines.erase(it);
      return 0;
    }
    CORE_LOG_DEBUG("Beginning load for asset ID: {} (Type: {}, Path: {})", asset_id, asset_type, file_path.string());

    asset* loading_asset = &it->loading_asset;
    loading_asset->path_hash = hash;
    state_it->second.handle_event(asset_event::LOAD_REQUESTED, loading_asset);

    CORE_LOG_TRACE("Executing load operation for asset ID: {}", loading_asset->id);
    auto itr = std::ranges::find_if(asset_pipelines, [id = loading_asset->id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(itr != asset_pipelines.end(), "Loading asset not found in asset pipelines");

    itr->pipeline->start_load(
      thread_pool, &itr->loading_asset,
      [this, id = itr->loading_asset.id](asset* asset_ptr) {
        OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load success callback");
        successful_pipelines.push(id);
      },
      [this, id = itr->loading_asset.id](asset* asset_ptr, const std::string& error_msg) {
        OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load failure callback");
        failed_pipelines.push(id);
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
      // pending_unloads.push(asset_id);
      return;
    }
    CORE_LOG_DEBUG("Unloading asset ID: {}", asset_id);

    state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    auto it = loaded_assets.find(asset_id);
    OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);

    asset::type asset_type = it->second.asset_type;
    auto pl_itr = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(asset_type, this),
                                                                  /// create a copy here? or should this be a pointer?
                                                                  .loading_asset = std::move(it->second),
                                                                });
    OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    CORE_LOG_TRACE("Executing unload operation for asset ID: {}", it->second.id);
    auto itr = std::ranges::find_if(asset_pipelines, [id = it->second.id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(itr != asset_pipelines.end(), "Unloading asset not found in asset pipelines");

    loaded_assets.erase(it);
    itr->pipeline->start_unload(
      thread_pool, &itr->loading_asset,
      [this, id = itr->loading_asset.id](asset* asset_ptr) {
        OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload success callback");
        successful_pipelines.push(id);
      },
      [this, id = itr->loading_asset.id](asset* asset_ptr, const std::string& error_msg) {
        OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload failure callback");
        failed_pipelines.push(id);
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

  natural_t asset_handler::get_asset_id_by_path_hash(natural_t path_hash) const {
    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == path_hash) {
        return a.id;
      }
    }

    for (const auto& ctx : asset_pipelines) {
      if (ctx.loading_asset.path_hash == path_hash) {
        return ctx.loading_asset.id;
      }
    }

    return 0;
  }

  asset* asset_handler::find_asset_by_path(const filepath& file_path) const {
    natural_t hash = FNV(std::filesystem::absolute(file_path).string());

    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == hash) {
        return const_cast<asset*>(&a);
      }
    }

    for (const auto& ctx : asset_pipelines) {
      if (ctx.loading_asset.path_hash == hash) {
        return const_cast<asset*>(&ctx.loading_asset);
      }
    }

    return nullptr;
  }

  void asset_handler::on_asset_loaded(natural_t id) {
    CORE_LOG_DEBUG("Asset loaded successfully (ID: {})", id);

    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Loaded asset not found in loading assets");

    auto itr = loaded_assets.insert({ id, std::move(pending_itr->loading_asset) });
    OTHER_ASSERT(itr.second, "Failed to insert loaded asset into loaded assets map");

    pending_itr->pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    register_asset_in_filesystem(&itr.first->second);
    state_itr->second.handle_event(asset_event::LOAD_COMPLETED);
  }

  void asset_handler::on_asset_load_failed(natural_t id) {
    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Failed asset not found in loading assets");

    CORE_LOG_ERROR("Failed to load asset (ID: {}): {}", id, pending_itr->pipeline->get_last_error());
    pending_itr->pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    state_itr->second.handle_event(asset_event::ERROR_EVENT);
  }

  void asset_handler::on_asset_unloaded(natural_t id) {
    CORE_LOG_DEBUG("Asset unloaded successfully (ID: {})", id);

    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Unloaded asset not found in loading assets");

    auto it = unloaded_assets.insert({ id, std::move(pending_itr->loading_asset) });
    OTHER_ASSERT(it.second, "Failed to insert unloaded asset into unloaded assets map");

    unregister_asset_in_filesystem(&it.first->second);
    state_itr->second.handle_event(asset_event::UNLOAD_COMPLETED);

    pending_itr->pipeline = nullptr;
    asset_pipelines.erase(pending_itr);
  }

  void asset_handler::on_asset_unload_failed(natural_t id) {
    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Failed asset not found in loading assets");

    CORE_LOG_ERROR("Failed to unload asset (ID: {}): {}", id, pending_itr->pipeline->get_last_error());
    pending_itr->pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

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
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register_asset_in_filesystem");
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      return;
    }

    const filepath& asset_path = asset_ptr->load_path;
    if (asset_path.empty()) {
      return;
    }

    CORE_LOG_DEBUG("Registering asset in filesystem: {}", asset_path.string());
    filepath abs_asset_path = std::filesystem::absolute(asset_path);
    std::string asset_path_str = asset_path.string();

    constexpr static std::string_view kAssetMountPrefix = "assets";
    std::string dir = asset_ptr->get_filesystem_directory();

    std::string virtual_path = std::string{ kAssetMountPrefix } + std::string{ file_system::kPathSeparator } + dir;

    ref<directory> mount = fs->get_or_create_mount(kAssetMountPrefix);
    OTHER_ASSERT(mount != nullptr, "Failed to get or create mount '{}' for asset: {}", virtual_path, asset_path.string());

    ref<directory> dir_handle = mount->get_child_directory(dir);
    if (dir_handle == nullptr) {
      dir_handle = mount->add_child_directory(dir, mount->absolute_path() / dir);
    }
    OTHER_ASSERT(dir_handle != nullptr, "Failed to get or create directory '{}' in mount '{}' for asset: {}", dir, virtual_path, asset_path.string());

    if (mount == nullptr) {
      CORE_LOG_ERROR("Failed to get or create default mount '{}' for asset: {}", default_mount, asset_path.string());
      return;
    }

    ref<file_handle> file_handle = make_ref<local_file>(abs_asset_path);
    OTHER_ASSERT(file_handle != nullptr, "Failed to create file handle for asset: {}", asset_path.string());
    dir_handle->add_file(file_handle);
    CORE_LOG_INFO("Registered asset file for [{}] :\n{}", asset_ptr->asset_type, file_handle->to_string());
  }

  void asset_handler::unregister_asset_in_filesystem(const asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unregister_asset_in_filesystem");
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      return;
    }

    const filepath& asset_path = asset_ptr->load_path;
    if (asset_path.empty()) {
      return;
    }

    CORE_LOG_DEBUG("Unregistering asset from filesystem: {}", asset_path.string());
    filepath abs_asset_path = std::filesystem::absolute(asset_path);
    std::string asset_path_str = asset_path.string();

    constexpr static std::string_view kAssetMountPrefix = "assets";
    std::string dir = asset_ptr->get_filesystem_directory();

    ref<directory> mount = fs->get_mount(kAssetMountPrefix);
    OTHER_ASSERT(mount != nullptr, "Failed to get mount '{}' for asset: {}", kAssetMountPrefix, asset_path.string());

    ref<directory> dir_handle = mount->get_child_directory(dir);
    if (dir_handle == nullptr) {
      CORE_LOG_WARN("Directory '{}' not found in mount '{}' for asset: {}", dir, kAssetMountPrefix, asset_path.string());
      return;
    }
    dir_handle->remove_file_by_path(asset_path);
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