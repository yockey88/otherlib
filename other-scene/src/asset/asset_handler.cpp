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

#include "scene/scene.hpp"

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

  void asset_handler::begin_unload() {
    remove_after_unload = true;
    for (auto it = loaded_assets.begin(); it != loaded_assets.end();) {
      /// add unloading pipelines for each asset to be unloaded
      natural_t asset_id = it->first;
      it = begin_unload(asset_id);
    }
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

    if (remove_after_unload) {
      for (auto it = unloaded_assets.begin(); it != unloaded_assets.end();) {
        all_assets.erase(std::ranges::find(all_assets, it->first));
        it = unloaded_assets.erase(it);
      }

      if (all_assets.empty()) {
        remove_after_unload = false;
        events.trigger_event("assets.all-assets-unloaded");
      }
    }
  }

  natural_t asset_handler::load_asset(const filepath& file_path, load_completion_callback on_complete) {
    PROFILE_SECTION("asset_handler::load_asset");

    std::string extension = file_path.extension().string();
    asset::type asset_type = asset::get_type_from_extension(extension);
    if (asset_type == asset::type::EMPTY) {
      CORE_LOG_ERROR("Unsupported asset file extension: {}", extension);
      return 0;
    }

    bool exists = std::filesystem::exists(file_path);
    if (!exists) {
      CORE_LOG_ERROR("Asset file does not exist: {}", file_path.string());
      return 0;
    }

    auto absolute_path = std::filesystem::absolute(file_path);
    natural_t hash = FNV(absolute_path.string());
    if (auto itr = std::ranges::find_if(loaded_assets, [hash](const auto& pair) { return pair.second.path_hash == hash; });
        itr != loaded_assets.end()) {
      CORE_LOG_WARN("Asset already loaded for path: {}. Returning existing asset ID: {}", file_path.string(), itr->second.id);
      return itr->second.id;
    }

    natural_t asset_id = get_next_asset_id();
    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                              .pipeline = asset_pipeline::get_asset_pipeline(events, this, asset_type),
                                                              .loading_asset = asset{
                                                                .asset_type = asset_type,
                                                                .id = asset_id,
                                                                .path_hash = hash,
                                                                .load_path = file_path,
                                                                // clang-format off
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, 
                                                                                asset::get_filesystem_directory(asset_type), file_path.filename().string()),
                                                                // clang-format on
                                                                .absolute_path = std::filesystem::absolute(absolute_path),
                                                              },
                                                              .on_complete = on_complete,
                                                            });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(asset_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for asset ID: {}", asset_id);
    CORE_LOG_DEBUG("Beginning load for asset ID: {} (Type: {}, Path: {})", asset_id, asset_type, file_path.string());

    begin_load(it, state_it);

    return asset_id;
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

  natural_t asset_handler::add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    CORE_LOG_DEBUG("Adding model source asset with name: {} (vertex count: {}, index count: {})", name, vertices.size(), indices.size());
    natural_t model_id = get_next_asset_id();

    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                              .pipeline = asset_pipeline::get_model_source_pipeline(events, this, name, vertices, indices),
                                                              .loading_asset = {
                                                                .asset_type = asset::type::MODEL_SOURCE,
                                                                .id = model_id,
                                                                .path_hash = 0,
                                                                .load_path = filepath{},
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::MODEL_SOURCE), name + ".modelsource"),
                                                                .absolute_path = filepath{},
                                                              },
                                                            });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(model_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for model source asset ID: {}", model_id);
    CORE_LOG_DEBUG("Beginning add_model_source_asset for asset ID: {} (Name: {})", model_id, name);

    begin_load(it, state_it);

    return model_id;
  }

  natural_t asset_handler::add_scene_asset(scene* scene_ptr, opt<filepath> scene_path) {
    CORE_LOG_DEBUG("Adding scene asset with name: {} (scene pointer: {})", scene_path.has_value() ? scene_path->string() : "<no path>", static_cast<void*>(scene_ptr));
    natural_t scene_id = get_next_asset_id();

    filepath p = scene_path.value_or(filepath{});
    filepath absolute_path = std::filesystem::absolute(p);
    std::string name = p.filename().stem().string();
    if (name.empty()) {
      name = std::format("scene_{}", scene_id);
    }

    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                              .pipeline = asset_pipeline::get_scene_pipeline(events, this, scene_ptr),
                                                              .loading_asset = {
                                                                .asset_type = asset::type::SCENE,
                                                                .id = scene_id,
                                                                .path_hash = 0,
                                                                .load_path = scene_path.value_or(filepath{}),
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::SCENE), name + ".scene"),
                                                                .absolute_path = absolute_path,
                                                              },
                                                            });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(scene_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for scene asset ID: {}", scene_id);
    CORE_LOG_DEBUG("Beginning add_scene_asset for asset ID: {} (Name: {})", scene_id, it->loading_asset.virtual_path);

    begin_load(it, state_it);

    return scene_id;
  }

  natural_t asset_handler::add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition) {
    CORE_LOG_DEBUG("Adding rendering pipeline asset with name: {}", name);
    natural_t pl_id = get_next_asset_id();

    std::string pl_name = std::string{ name };
    auto it = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                              .pipeline = asset_pipeline::get_rendering_pipeline_pipeline(events, this, definition),
                                                              .loading_asset = {
                                                                .asset_type = asset::type::RENDERING_PIPELINE,
                                                                .id = pl_id,
                                                                .path_hash = 0,
                                                                .load_path = filepath{},
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::RENDERING_PIPELINE), pl_name + ".orpl"),
                                                                .absolute_path = filepath{},
                                                              },
                                                            });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(pl_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for rendering pipeline asset ID: {}", pl_id);
    CORE_LOG_DEBUG("Beginning add_rendering_pipeline_asset for asset ID: {} (Name: {})", pl_id, it->loading_asset.virtual_path);

    begin_load(it, state_it);
    return pl_id;
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
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(events, this, asset_type),
                                                                  /// create a copy here? or should this be a pointer?
                                                                  .loading_asset = std::move(it->second),
                                                                });
    OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    loaded_assets.erase(it);
    pl_itr->pipeline->start_unload(
      executor, &pl_itr->loading_asset,
      std::bind_front(&asset_handler::notify_asset_load_complete, this),
      std::bind_front(&asset_handler::notify_asset_load_failed, this)
    );
  }

  asset* asset_handler::get_asset(natural_t asset_id) {
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return &it->second;
    }

    auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& a) { return a.loading_asset.id == asset_id; });
    if (it != asset_pipelines.end()) {
      return &it->loading_asset;
    }

    return nullptr;
  }

  std::span<const natural_t> asset_handler::get_all_asset_ids() const {
    return std::span<const natural_t>{ all_assets.begin(), all_assets.end() };
  }

  asset_state asset_handler::get_asset_state(natural_t asset_id) const {
    auto it = asset_states.find(asset_id);
    if (it != asset_states.end()) {
      return it->second.get_current_state();
    }
    return asset_state::ERROR_STATE;
  }

  natural_t asset_handler::get_asset_hash(natural_t asset_id) const {
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return it->second.path_hash;
    }
    if (auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& a) { return a.loading_asset.id == asset_id; });
        it != asset_pipelines.end()) {
      if (it->loading_asset.id == asset_id) {
        return it->loading_asset.path_hash;
      }
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

  void asset_handler::begin_load(std::deque<pipeline_context>::iterator pipeline_it, std::unordered_map<natural_t, asset_state_machine>::iterator state_it) {
    OTHER_ASSERT(pipeline_it != asset_pipelines.end(), "Invalid pipeline iterator in begin_load");
    OTHER_ASSERT(state_it != asset_states.end(), "Invalid state machine iterator in begin_load");

    asset* loading_asset = &pipeline_it->loading_asset;

    CORE_LOG_TRACE("Executing load operation for asset ID: {}", loading_asset->id);
    state_it->second.handle_event(asset_event::LOAD_REQUESTED, loading_asset);
    pipeline_it->pipeline->start_load(
      executor, &pipeline_it->loading_asset,
      std::bind_front(&asset_handler::notify_asset_load_complete, this),
      std::bind_front(&asset_handler::notify_asset_load_failed, this)
    );

    if (std::ranges::find(all_assets, loading_asset->id) == all_assets.end()) {
      all_assets.push_back(loading_asset->id);
    }
  }

  std::unordered_map<natural_t, asset>::iterator asset_handler::begin_unload(natural_t asset_id) {
    auto state_itr = asset_states.find(asset_id);
    if (state_itr == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return loaded_assets.end();
    }

    if (state_itr->second.get_current_state() == asset_state::UNLOADING) {
      return loaded_assets.end();
    }

    CORE_LOG_DEBUG("Beginning unload for asset ID: {}", asset_id);

    state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    auto it = loaded_assets.find(asset_id);
    OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);

    asset::type asset_type = it->second.asset_type;
    auto pl_itr = asset_pipelines.insert(asset_pipelines.end(), pipeline_context{
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(events, this, asset_type),
                                                                  .loading_asset = std::move(it->second),
                                                                });
    OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto rit = loaded_assets.erase(it);
    pl_itr->pipeline->start_unload(
      executor, &pl_itr->loading_asset,
      std::bind_front(&asset_handler::notify_asset_load_complete, this),
      std::bind_front(&asset_handler::notify_asset_load_failed, this)
    );
    return rit;
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

  void asset_handler::notify_asset_load_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load success callback");
    successful_pipelines.push(asset_ptr->id);
  }

  void asset_handler::notify_asset_load_failed(asset* asset_ptr, const std::string& error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load failure callback");
    failed_pipelines.push(asset_ptr->id);
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

    state_itr->second.handle_event(asset_event::LOAD_FAILED);
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

    state_itr->second.handle_event(asset_event::UNLOAD_FAILED);
  }

  void asset_handler::register_asset_in_filesystem(const asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register_asset_in_filesystem");
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      return;
    }

    /// generated/created assets are full virtual, where as all assets are registered under
    //    a virtual path with it's local path attached to the asset itself
    bool full_virtual_file = asset_ptr->load_path.empty();
    /// assets are first-and-foremost identified by virtual path
    OTHER_ASSERT(!asset_ptr->virtual_path.empty(), "Asset virtual path is empty for asset ID: {}", asset_ptr->id);

    /// assets must have a mount name
    resolved_path resolved = fs->resolve_path(asset_ptr->virtual_path);
    OTHER_ASSERT(!resolved.mount_name.empty(), "Failed to resolve mount for asset virtual path: {}", asset_ptr->virtual_path);

    ref<directory> mount = fs->get_or_create_mount(default_mount);
    OTHER_ASSERT(mount != nullptr, "Failed to get or create mount '{}' for asset: {}", asset_ptr->virtual_path, asset_ptr->id);

    std::string curr_virtual_path = mount->absolute_path().string() + std::string{ file_system::kPathSeparator };
    ref<directory> dir_handle = mount;
    for (const auto& piece : resolved.relative_path_components) {
      OTHER_ASSERT(dir_handle != nullptr, "Directory handle is null after creation or retrieval for piece '{}' in asset virtual path: {}", piece, asset_ptr->virtual_path);
      auto next = dir_handle->get_child_directory(piece);
      if (next == nullptr) {
        dir_handle = dir_handle->add_child_directory(piece);
      } else {
        dir_handle = next;
      }
      curr_virtual_path += std::format("/{}/", piece);
    }

    OTHER_ASSERT(dir_handle != nullptr, "Final directory handle is null for asset virtual path: {}", asset_ptr->virtual_path);
    CORE_LOG_INFO("Directory for asset ID {}: {}", asset_ptr->id, dir_handle->to_string());

    file_type type = full_virtual_file ? file_type::VIRTUAL : file_type::LOCAL;
    ref<file_handle> file_handle = nullptr;
    if (type == file_type::LOCAL) {
      OTHER_ASSERT(!asset_ptr->load_path.empty(), "Asset load path is empty for local file registration for asset ID: {}", asset_ptr->id);
      auto file = fs->register_local_file(asset_ptr->load_path);
      OTHER_ASSERT(file != nullptr, "Failed to register local file for asset load path: {}", asset_ptr->load_path.string());
    }

    file_handle = fs->create_asset_virtual_file(asset_ptr->virtual_path);
    if (type == file_type::LOCAL) {
      file_handle->set_absolute_path(asset_ptr->absolute_path);
    } else {
      file_handle->set_absolute_path(asset_ptr->virtual_path);
    }
    file_handle->set_virtual_path(asset_ptr->virtual_path);

    OTHER_ASSERT(file_handle != nullptr, "Failed to create file handle for asset: {}", asset_ptr->virtual_path);
    CORE_LOG_INFO("Asset file registered for asset [{}] of type [{}] :\n{}", asset_ptr->id, asset_ptr->asset_type, file_handle->to_string());
    dir_handle->add_file(file_handle);
  }

  void asset_handler::unregister_asset_in_filesystem(const asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unregister_asset_in_filesystem");

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem not available in asset handler for unregistering asset from filesystem");

    const filepath& asset_path = asset_ptr->load_path.empty() ? asset_ptr->virtual_path : asset_ptr->load_path;
    OTHER_ASSERT(!asset_path.empty(), "Asset path is empty for asset ID: {}", asset_ptr->id);

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