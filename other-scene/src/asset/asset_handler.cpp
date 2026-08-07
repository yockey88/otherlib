/**
 * \file asset/asset_handler.cpp
 **/
#include "asset/asset_handler.hpp"

#include <filesystem>
#include <ranges>
#include <span>

#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/subsystem.hpp"
#include "file/filesystem.hpp"
#include "file/path_helpers.hpp"

#include "renderer/pipeline_definition.hpp"

#include "scene/scene.hpp"

#include "asset.hpp"

namespace other {

  void asset_handler::begin_unload() {
    PROFILE_SECTION("asset_handler::begin_unload");
    remove_after_unload = true;
    for (auto it = loaded_assets.begin(); it != loaded_assets.end();) {
      /// add unloading pipelines for each asset to be unloaded
      natural_t asset_id = it->first;
      it = begin_unload(asset_id);
    }
  }

  void asset_handler::update_pipelines() {
    PROFILE_SECTION("asset_handler::update_pipelines");

    {
      PROFILE_SECTION("asset_handler::update_pipelines--poll");
      for (auto& pl : asset_pipelines) {
        pl.second.pipeline->poll();
      }
    }

    {
      PROFILE_SECTION("asset_handler::update_pipelines--completions");
      while (!successful_pipelines.empty()) {
        natural_t id = successful_pipelines.front();
        successful_pipelines.pop();

        switch (get_asset_state(id)) {
          case asset_state::LOADING:
          case asset_state::REFRESHING_LOAD:
            on_asset_loaded(id);
            break;
          case asset_state::UNLOADING:
          case asset_state::REFRESHING_UNLOAD:
            on_asset_unloaded(id);
            break;
          default:
            OTHER_ASSERT(false, "Asset ID {} in unexpected state after successful pipeline completion", id);
        }
      }

      while (!failed_pipelines.empty()) {
        natural_t id = failed_pipelines.front();
        failed_pipelines.pop();

        switch (get_asset_state(id)) {
          case asset_state::LOADING:
          case asset_state::REFRESHING_LOAD:
            on_asset_load_failed(id);
            break;
          case asset_state::UNLOADING:
          case asset_state::REFRESHING_UNLOAD:
            on_asset_unload_failed(id);
            break;
          default:
            OTHER_ASSERT(false, "Asset ID {} in unexpected state after failed pipeline completion", id);
        }
      }
    }

    {
      PROFILE_SECTION("asset_handler::update_pipelines--pending-unloads");
      for (size_t remaining = pending_unloads.size(); remaining > 0; --remaining) {
        const natural_t id = pending_unloads.front();
        pending_unloads.pop();

        if (asset_loaded(id)) {
          unload_asset(id);
          continue;
        }

        switch (get_asset_state(id)) {
          case asset_state::LOADING:
          case asset_state::REFRESHING_UNLOAD:
          case asset_state::REFRESHING_LOAD:
            /// still in flight
            pending_unloads.push(id);
            break;
          default:
            /// failed, already unloading/unloaded, or gone
            break;
        }
      }
    }
  }

  void asset_handler::resolve_roots(std::span<const filepath> roots) {
    PROFILE_SECTION("asset_handler::resolve_roots");
    /// clean slate: resolve() rebuilds domains and root_ids; pending refresh marks from
    //  a previous project must not leak into the new plan (stable_ids are deterministic)
    needs_refresh.clear();
    snapshot = resolver.resolve(roots);
    execute_plan();
  }

  void asset_handler::re_resolve(const filepath& changed) {
    PROFILE_SECTION("asset_handler::re_resolve");
    const resolve_delta delta = resolver.re_resolve(snapshot, changed);
    if (delta.empty()) {
      return;
    }
    CORE_LOG_DEBUG("asset delta for '{}': {}", changed.string(), delta.to_string());

    for (const natural_t dead : delta.removed) {
      if (const natural_t id = runtime_id(dead); id != 0) {
        unload_asset(id);
        runtime_by_stable.erase(dead);
      }
    }

    for (const natural_t stable : delta.modified) {
      needs_refresh.insert(stable);
    }
    for (const natural_t stable : delta.affected_parents) {
      needs_refresh.insert(stable);
    }

    execute_plan();
  }

  natural_t asset_handler::load_asset(const filepath& file_path, load_completion_callback on_complete) {
    PROFILE_SECTION("asset_handler::load_asset");

    std::string extension = file_path.extension().string();
    asset::type asset_type = asset::get_type_from_extension(extension);
    std::string name = file_path.filename().string();
    if (asset_type == asset::type::EMPTY) {
      CORE_LOG_ERROR("Unsupported asset file extension: {}", extension);
      return 0;
    }

    if (asset_type == asset::ASSET_DECLARATION) {
      // have to read file and see what it is (.toml)
      asset_type = asset::get_type_from_declaration(file_path);
      if (asset_type == asset::type::EMPTY) {
        CORE_LOG_ERROR("Failed to determine asset type from declaration file: {}", file_path.string());
        return 0;
      }

      name = asset::get_name_from_declaration(file_path);
    }

    bool exists = std::filesystem::exists(file_path);
    if (!exists) {
      CORE_LOG_ERROR("Asset file does not exist: {}", file_path.string());
      return 0;
    }

    auto absolute_path = std::filesystem::absolute(file_path);
    natural_t hash = FNV(absolute_path.string());
    if (auto itr = std::ranges::find_if(loaded_assets, [hash](const auto& pair) { return pair.second.path_hash == hash; }); itr != loaded_assets.end()) {
      return itr->second.id;
    }
    /// a plan-dispatched load and a direct load (scene instantiation resolving the same
    //  model) can race; a second pipeline for the same file would double-load the payload
    if (auto itr = std::ranges::find_if(asset_pipelines, [hash](const auto& pair) { return pair.second.loading_asset.path_hash == hash; }); itr != asset_pipelines.end()) {
      if (on_complete != nullptr) {
        CORE_LOG_WARN("Asset '{}' is already loading (ID: {}); completion callback dropped", file_path.string(), itr->second.loading_asset.id);
      }
      return itr->second.loading_asset.id;
    }

    natural_t asset_id = get_next_asset_id();
    auto [it, success] = asset_pipelines.insert_or_assign(asset_id, pipeline_context{
                                                                      .pipeline = asset_pipeline::get_asset_pipeline(&events, this, asset_type),
                                                                      .loading_asset = asset{
                                                                        .asset_type = asset_type,
                                                                        .id = asset_id,
                                                                        .stable_id = stable_id_for(virtualize(absolute_path)),
                                                                        .path_hash = hash,
                                                                        .load_path = file_path,
                                                                        // clang-format off
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, 
                                                                                asset::get_filesystem_directory(asset_type), name),
                                                                        // clang-format on
                                                                        .absolute_path = std::filesystem::absolute(absolute_path),
                                                                      },
                                                                      .on_complete = on_complete,
                                                                    });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(asset_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for asset ID: {}", asset_id);
    CORE_LOG_DEBUG("Beginning load for asset ID: {} (Type: {}, Path: {})", asset_id, asset_type, file_path.string());

    runtime_by_stable[it->second.loading_asset.stable_id] = asset_id;
    begin_load(it, state_it);

    return asset_id;
  }

  natural_t asset_handler::load_asset(const std::string_view engine_path, load_completion_callback on_complete) {
    PROFILE_SECTION("asset_handler::load_asset");
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

  natural_t asset_handler::add_model_source_asset(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices) {
    PROFILE_SECTION("asset_handler::add_model_source_asset");
    CORE_LOG_DEBUG("Adding model source asset with name: {} (vertex count: {}, index count: {})", name, vertices.size(), indices.size());
    natural_t model_id = get_next_asset_id();

    auto [it, inserted] = asset_pipelines.emplace(model_id, pipeline_context{
                                                              .pipeline = asset_pipeline::get_model_source_pipeline(&events, this, name, vertices, indices),
                                                              .loading_asset = {
                                                                .asset_type = asset::type::MODEL_SOURCE,
                                                                .id = model_id,
                                                                .stable_id = stable_id_for(std::format("mem:{}/{}", asset::get_filesystem_directory(asset::type::MODEL_SOURCE), name)),
                                                                .path_hash = 0,
                                                                .load_path = filepath{},
                                                                .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::MODEL_SOURCE), name + ".modelsource"),
                                                                .absolute_path = filepath{},
                                                              },
                                                            });
    OTHER_ASSERT(inserted, "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(model_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for model source asset ID: {}", model_id);
    CORE_LOG_DEBUG("Beginning add_model_source_asset for asset ID: {} (Name: {})", model_id, name);

    runtime_by_stable[it->second.loading_asset.stable_id] = model_id;
    begin_load(it, state_it);

    return model_id;
  }

  natural_t asset_handler::add_scene_asset(scene* scene_ptr, opt<filepath> scene_path) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null");
    PROFILE_SECTION("asset_handler::add_scene_asset");
    CORE_LOG_DEBUG("Adding scene asset with name: {} (scene pointer: {})", scene_path.has_value() ? scene_path->string() : "<no path>", static_cast<void*>(scene_ptr));
    natural_t scene_id = get_next_asset_id();

    filepath p = scene_path.value_or(filepath{});
    filepath absolute_path = std::filesystem::absolute(p);
    std::string name = p.filename().stem().string();
    if (name.empty()) {
      name = std::format("scene_{}", scene_id);
    }

    asset_pipelines.emplace(scene_id, pipeline_context{
                                        .pipeline = asset_pipeline::get_scene_pipeline(&events, this, scene_ptr),
                                        .loading_asset = {
                                          .asset_type = asset::type::SCENE,
                                          .id = scene_id,
                                          .stable_id = stable_id_for(std::format("mem:{}/{}", asset::get_filesystem_directory(asset::type::SCENE), name)),
                                          // since we are loading an already existing scene into system
                                          // we use 0 here to tell the pipeline to set scene's asset id
                                          // so scene can be found when load is finished
                                          .path_hash = 0,
                                          .load_path = p,
                                          .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::SCENE), name + ".scene"),
                                          .absolute_path = absolute_path,
                                        },
                                      });
    auto it = asset_pipelines.find(scene_id);
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert scene asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(scene_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for scene asset ID: {}", scene_id);
    CORE_LOG_DEBUG("Beginning add_scene_asset for asset ID: {} (Name: {})", scene_id, it->second.loading_asset.virtual_path.string());

    runtime_by_stable[it->second.loading_asset.stable_id] = scene_id;
    begin_load(it, state_it);

    return scene_id;
  }

  natural_t asset_handler::add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition) {
    PROFILE_SECTION("asset_handler::add_rendering_pipeline_asset");
    CORE_LOG_DEBUG("Adding rendering pipeline asset with name: {}", name);
    natural_t pl_id = get_next_asset_id();

    std::string pl_name = std::string{ name };
    auto [it, inserted] = asset_pipelines.emplace(pl_id, pipeline_context{
                                                           .pipeline = asset_pipeline::get_rendering_pipeline_pipeline(&events, this, definition),
                                                           .loading_asset = {
                                                             .asset_type = asset::type::RENDERING_PIPELINE,
                                                             .id = pl_id,
                                                             .stable_id = stable_id_for(std::format("mem:{}/{}", asset::get_filesystem_directory(asset::type::RENDERING_PIPELINE), pl_name)),
                                                             .path_hash = 0,
                                                             .load_path = filepath{},
                                                             .virtual_path = std::format("{}{}{}/{}", default_mount, file_system::kPathSeparator, asset::get_filesystem_directory(asset::type::RENDERING_PIPELINE), pl_name + ".orpl"),
                                                             .absolute_path = filepath{},
                                                           },
                                                         });
    OTHER_ASSERT(it != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto [state_it, state_inserted] = asset_states.emplace(pl_id, asset_state_machine{});
    OTHER_ASSERT(state_inserted, "Failed to insert asset state machine for rendering pipeline asset ID: {}", pl_id);
    CORE_LOG_DEBUG("Beginning add_rendering_pipeline_asset for asset ID: {} (Name: {})", pl_id, it->second.loading_asset.virtual_path.string());

    runtime_by_stable[it->second.loading_asset.stable_id] = pl_id;
    begin_load(it, state_it);
    return pl_id;
  }

  void asset_handler::unload_asset(natural_t asset_id) {
    PROFILE_SECTION("asset_handler::unload_asset");
    auto state_itr = asset_states.find(asset_id);
    if (state_itr == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return;
    }

    if (state_itr->second.get_current_state() == asset_state::UNLOADING) {
      CORE_LOG_WARN("Asset ID {} is already unloading. Ignoring duplicate unload request.", asset_id);
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

    asset::type asset_type = it->second.asset_type;
    auto [pl_itr, inserted] = asset_pipelines.emplace(asset_id, pipeline_context{
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(&events, this, asset_type),
                                                                  /// create a copy here? or should this be a pointer?
                                                                  .loading_asset = std::move(it->second),
                                                                });
    OTHER_ASSERT(inserted, "Failed to insert asset into loading assets list");

    loaded_assets.erase(it);
    pl_itr->second.pipeline->start_unload(
      jobs, &pl_itr->second.loading_asset,
      std::bind_front(&asset_handler::notify_asset_unload_complete, this),
      std::bind_front(&asset_handler::notify_asset_unload_failed, this));
  }

  void asset_handler::reload_asset(natural_t asset_id) {
    PROFILE_SECTION("asset_handler::reload_asset");
    if (!asset_loaded(asset_id)) {
      CORE_LOG_WARN("Asset ID {} is not currently loaded. Cannot reload asset that is not loaded.", asset_id);
      return;
    }

    auto state_it = asset_states.find(asset_id);
    if (state_it == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return;
    }
    OTHER_ASSERT(state_it->second.get_current_state() == asset_state::LOADED, "Asset ID {} is not loaded, cannot refresh", asset_id);

    asset asset_to_reload;
    {
      auto it = loaded_assets.find(asset_id);
      OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);
      asset_to_reload = std::move(it->second);
      loaded_assets.erase(it);
    }

    auto [pl_itr, inserted] = asset_pipelines.emplace(asset_id, pipeline_context{
                                                                  .pipeline = asset_pipeline::get_asset_pipeline(&events, this, asset_to_reload.asset_type),
                                                                  .loading_asset = std::move(asset_to_reload),
                                                                });
    OTHER_ASSERT(inserted, "Failed to insert asset into loading assets list");

    state_it->second.handle_event(asset_event::REFRESH_REQUESTED, &pl_itr->second.loading_asset);

    pl_itr->second.pipeline->start_unload(
      jobs, &pl_itr->second.loading_asset,
      std::bind_front(&asset_handler::notify_asset_unload_complete, this),
      std::bind_front(&asset_handler::notify_asset_unload_failed, this));
  }

  asset* asset_handler::get_asset(natural_t asset_id) {
    PROFILE_SECTION("asset_handler::get_asset");
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return &it->second;
    }

    auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& a) { return a.second.loading_asset.id == asset_id; });
    if (it != asset_pipelines.end()) {
      return &it->second.loading_asset;
    }

    return nullptr;
  }

  asset* asset_handler::get_asset_by_virtual_path(const filepath& virtual_path) {
    PROFILE_SECTION("asset_handler::get_asset_by_virtual_path");
    if (auto it = std::ranges::find_if(loaded_assets, [&virtual_path](const auto& pair) { return pair.second.virtual_path == virtual_path; }); it != loaded_assets.end()) {
      return &it->second;
    }

    auto it = std::ranges::find_if(asset_pipelines, [&virtual_path](const auto& a) { return a.second.loading_asset.virtual_path == virtual_path; });
    if (it != asset_pipelines.end()) {
      return &it->second.loading_asset;
    }

    return nullptr;
  }

  ostd::vector<asset*> asset_handler::get_assets_of_type(asset::type type) {
    PROFILE_SECTION("asset_handler::get_assets_of_type");
    return loaded_assets |
      std::views::values |
      std::views::filter([type](asset& a) { return a.asset_type == type; }) |
      std::views::transform([](asset& a) { return &a; }) |
      std::ranges::to<ostd::vector<asset*>>();
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
    PROFILE_SECTION("asset_handler::get_asset_hash");
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return it->second.path_hash;
    }
    if (auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& a) { return a.second.loading_asset.id == asset_id; }); it != asset_pipelines.end()) {
      if (it->second.loading_asset.id == asset_id) {
        return it->second.loading_asset.path_hash;
      }
    }

    return 0;
  }

  natural_t asset_handler::get_asset_id_from_path(const filepath& path) const {
    return get_asset_id_by_path_hash(FNV(std::filesystem::absolute(path).string()));
  }

  natural_t asset_handler::get_asset_id_by_path_hash(natural_t path_hash) const {
    PROFILE_SECTION("asset_handler::get_asset_id_by_path_hash");
    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == path_hash) {
        return a.id;
      }
    }

    for (const auto& [id, ctx] : asset_pipelines) {
      if (ctx.loading_asset.path_hash == path_hash) {
        return ctx.loading_asset.id;
      }
    }

    return 0;
  }

  opt<filepath> asset_handler::get_local_asset_path(natural_t asset_id) const {
    PROFILE_SECTION("asset_handler::get_local_asset_path");
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return it->second.absolute_path;
    }
    if (auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& pair) { return pair.second.loading_asset.id == asset_id; }); it != asset_pipelines.end()) {
      return it->second.loading_asset.absolute_path;
    }

    return std::nullopt;
  }

  opt<filepath> asset_handler::get_virtual_asset_path(natural_t asset_id) const {
    PROFILE_SECTION("asset_handler::get_virtual_asset_path");
    if (auto it = loaded_assets.find(asset_id); it != loaded_assets.end()) {
      return it->second.virtual_path;
    }
    if (auto it = std::ranges::find_if(asset_pipelines, [asset_id](const auto& pair) { return pair.second.loading_asset.id == asset_id; }); it != asset_pipelines.end()) {
      return it->second.loading_asset.virtual_path;
    }

    return std::nullopt;
  }

  void asset_handler::begin_load(ostd::map<natural_t, pipeline_context>::iterator pipeline_it, ostd::unordered_map<natural_t, asset_state_machine>::iterator state_it) {
    OTHER_ASSERT(pipeline_it != asset_pipelines.end(), "Invalid pipeline iterator in begin_load");
    OTHER_ASSERT(state_it != asset_states.end(), "Invalid state machine iterator in begin_load");
    PROFILE_SECTION("asset_handler::begin_load");

    asset* loading_asset = &pipeline_it->second.loading_asset;

    CORE_LOG_TRACE("Executing load operation for asset ID: {}", loading_asset->id);
    state_it->second.handle_event(asset_event::LOAD_REQUESTED, loading_asset);
    pipeline_it->second.pipeline->start_load(
      jobs, &pipeline_it->second.loading_asset,
      std::bind_front(&asset_handler::notify_asset_load_complete, this),
      std::bind_front(&asset_handler::notify_asset_load_failed, this));

    if (std::ranges::find(all_assets, loading_asset->id) == all_assets.end()) {
      all_assets.push_back(loading_asset->id);
    }
  }

  ostd::unordered_map<natural_t, asset>::iterator asset_handler::begin_unload(natural_t asset_id) {
    PROFILE_SECTION("asset_handler::begin_unload");
    auto state_itr = asset_states.find(asset_id);
    if (state_itr == asset_states.end()) {
      CORE_LOG_ERROR("Asset state machine not found for asset ID: {}", asset_id);
      return loaded_assets.end();
    }

    if (state_itr->second.get_current_state() == asset_state::UNLOADING) {
      CORE_LOG_WARN("Asset ID: {} is already in the process of unloading.", asset_id);
      return loaded_assets.end();
    }

    CORE_LOG_DEBUG("Beginning unload for asset ID: {}", asset_id);
    CORE_LOG_DEBUG(" - asset type being unloaded: {}", get_asset(asset_id)->asset_type);

    state_itr->second.handle_event(asset_event::UNLOAD_REQUESTED);

    auto it = loaded_assets.find(asset_id);
    OTHER_ASSERT(it != loaded_assets.end(), "Asset not found in loaded assets map for asset ID: {}", asset_id);

    asset::type asset_type = it->second.asset_type;
    auto [pl_itr, success] = asset_pipelines.insert_or_assign(asset_id, pipeline_context{
                                                                          .pipeline = asset_pipeline::get_asset_pipeline(&events, this, asset_type),
                                                                          .loading_asset = std::move(it->second),
                                                                        });
    OTHER_ASSERT(pl_itr != asset_pipelines.end(), "Failed to insert asset into loading assets list");

    auto rit = loaded_assets.erase(it);
    pl_itr->second.pipeline->start_unload(
      jobs, &pl_itr->second.loading_asset,
      std::bind_front(&asset_handler::notify_asset_unload_complete, this),
      std::bind_front(&asset_handler::notify_asset_unload_failed, this));
    return rit;
  }

  asset* asset_handler::find_asset_by_path(const filepath& file_path) const {
    PROFILE_SECTION("asset_handler::find_asset_by_path");
    natural_t hash = FNV(std::filesystem::absolute(file_path).string());

    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == hash) {
        return const_cast<asset*>(&a);
      }
    }

    for (const auto& [key, ctx] : asset_pipelines) {
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

  void asset_handler::notify_asset_load_failed(asset* asset_ptr, const std::string_view error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in load failure callback");
    CORE_LOG_ERROR("Asset load failed for asset ID: {}: {}", asset_ptr->id, error_message);
    failed_pipelines.push(asset_ptr->id);
  }

  void asset_handler::notify_asset_unload_complete(asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload success callback");
    successful_pipelines.push(asset_ptr->id);
  }

  void asset_handler::notify_asset_unload_failed(asset* asset_ptr, const std::string_view error_message) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unload failure callback");
    CORE_LOG_ERROR("Asset unload failed for asset ID: {}: {}", asset_ptr->id, error_message);
    failed_pipelines.push(asset_ptr->id);
  }

  void asset_handler::on_asset_loaded(natural_t id) {
    PROFILE_SECTION("asset_handler::on_asset_loaded");
    CORE_LOG_DEBUG("Asset loaded successfully (ID: {})", id);
    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto prev_state = state_itr->second.get_current_state();
    OTHER_ASSERT(prev_state == asset_state::LOADING, "Asset ID {} completed a load outside LOADING", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.second.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Loaded asset not found in loading assets");

    auto [itr, success] = loaded_assets.insert({ id, std::move(pending_itr->second.loading_asset) });
    OTHER_ASSERT(success, "Failed to insert loaded asset into loaded assets map");
    if (pending_itr->second.on_complete) {
      pending_itr->second.on_complete(&itr->second);
    }

    pending_itr->second.pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    register_asset_in_filesystem(&itr->second);
    state_itr->second.handle_event(asset_event::LOAD_COMPLETED);

    on_planned_child_loaded(itr->second.stable_id);
  }

  void asset_handler::on_asset_load_failed(natural_t id) {
    PROFILE_SECTION("asset_handler::on_asset_load_failed");
    CORE_LOG_DEBUG("Asset load failed (ID: {})", id);

    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.second.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Failed asset not found in loading assets");

    const natural_t stable = pending_itr->second.loading_asset.stable_id;
    const std::string error = pending_itr->second.pipeline->get_last_error();
    CORE_LOG_ERROR("Failed to load asset (ID: {}): {}", id, error);

    pending_itr->second.pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    state_itr->second.handle_event(asset_event::LOAD_FAILED);
    asset_states.erase(state_itr);

    // fully remove it since error occurred
    auto it = unloaded_assets.find(id);
    if (it != unloaded_assets.end()) {
      unregister_asset_in_filesystem(&it->second);
      unloaded_assets.erase(it);
    }

    runtime_by_stable.erase(stable);
    on_planned_child_failed(stable, error);
  }

  void asset_handler::on_asset_unloaded(natural_t id) {
    PROFILE_SECTION("asset_handler::on_asset_unloaded");
    CORE_LOG_DEBUG("Asset unloaded successfully (ID: {})", id);

    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.second.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Unloaded asset not found in loading assets");

    auto prev_state = state_itr->second.get_current_state();
    state_itr->second.handle_event(asset_event::UNLOAD_COMPLETED);

    if (prev_state == asset_state::UNLOADING) {
      if (remove_after_unload) {
        auto it = unloaded_assets.insert({ id, std::move(pending_itr->second.loading_asset) });
        OTHER_ASSERT(it.second, "Failed to insert unloaded asset into unloaded assets map");
        unregister_asset_in_filesystem(&it.first->second);

        runtime_by_stable.erase(it.first->second.stable_id);
        asset_states.erase(state_itr);
      }

      pending_itr->second.pipeline = nullptr;
      asset_pipelines.erase(pending_itr);
    } else if (prev_state == asset_state::REFRESHING_UNLOAD) {
      // begin load again with the same asset data to refresh it
      begin_load(pending_itr, state_itr);
    } else {
      OTHER_ASSERT(false, "Asset ID {} in unexpected state after successful unload completion", id);
    }
  }

  void asset_handler::on_asset_unload_failed(natural_t id) {
    PROFILE_SECTION("asset_handler::on_asset_unload_failed");
    auto state_itr = asset_states.find(id);
    OTHER_ASSERT(state_itr != asset_states.end(), "Asset state machine not found for asset ID: {}", id);

    auto pending_itr = std::ranges::find_if(asset_pipelines, [id](const auto& a) { return a.second.loading_asset.id == id; });
    OTHER_ASSERT(pending_itr != asset_pipelines.end(), "Failed asset not found in loading assets");

    CORE_LOG_ERROR("Failed to unload asset (ID: {}): {}", id, pending_itr->second.pipeline->get_last_error());

    /// keep the asset alive past the pipeline erase; the iterator is dead after it
    asset failed_asset = std::move(pending_itr->second.loading_asset);
    pending_itr->second.pipeline = nullptr;
    asset_pipelines.erase(pending_itr);

    state_itr->second.handle_event(asset_event::UNLOAD_FAILED);
    asset_states.erase(state_itr);

    // fully remove it since error occurred
    runtime_by_stable.erase(failed_asset.stable_id);
    unregister_asset_in_filesystem(&failed_asset);
    auto it = unloaded_assets.find(id);
    if (it != unloaded_assets.end()) {
      unloaded_assets.erase(it);
    }
  }

  void asset_handler::register_asset_in_filesystem(const asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in register_asset_in_filesystem");
    PROFILE_SECTION("asset_handler::register_asset_in_filesystem");
    auto* fs = subsystem<file_system>::get();
    if (fs == nullptr) {
      return;
    }
    CORE_LOG_DEBUG("Registering asset in filesystem: {} (ID: {})", asset_ptr->virtual_path.string(), asset_ptr->id);

    /// generated/created assets are full virtual, where as all assets are registered under
    //    a virtual path with it's local path attached to the asset itself
    bool full_virtual_file = asset_ptr->load_path.empty();
    /// assets are first-and-foremost identified by virtual path
    OTHER_ASSERT(!asset_ptr->virtual_path.empty(), "Asset virtual path is empty for asset ID: {}", asset_ptr->id);

    /// assets must have a mount name
    resolved_path resolved = fs->resolve_path(asset_ptr->virtual_path.string());
    OTHER_ASSERT(!resolved.mount_name.empty(), "Failed to resolve mount for asset virtual path: {}", asset_ptr->virtual_path.string());

    CORE_LOG_DEBUG("Resolved path: {}", resolved);

    ref<directory> mount = fs->get_or_create_mount(default_mount);
    OTHER_ASSERT(mount != nullptr, "Failed to get or create mount '{}' for asset: {}", asset_ptr->virtual_path.string(), asset_ptr->id);

    std::string curr_virtual_path = mount->absolute_path().string() + std::string{ file_system::kPathSeparator };
    ref<directory> dir_handle = mount;
    for (const auto& piece : resolved.relative_path_components) {
      OTHER_ASSERT(dir_handle != nullptr, "Directory handle is null after creation or retrieval for piece '{}' in asset virtual path: {}", piece, asset_ptr->virtual_path.string());
      auto next = dir_handle->get_child_directory(piece);
      if (next == nullptr) {
        dir_handle = dir_handle->add_child_directory(piece);
      } else {
        dir_handle = next;
      }
      curr_virtual_path += std::format("/{}/", piece);
    }
    OTHER_ASSERT(dir_handle != nullptr, "Final directory handle is null for asset virtual path: {}", asset_ptr->virtual_path.string());

    file_type type = full_virtual_file ? file_type::VIRTUAL : file_type::LOCAL;
    ref<file_handle> file_handle = nullptr;
    if (type == file_type::LOCAL) {
      OTHER_ASSERT(!asset_ptr->load_path.empty(), "Asset load path is empty for local file registration for asset ID: {}", asset_ptr->id);
      auto file = fs->register_local_file(asset_ptr->load_path);
      OTHER_ASSERT(file != nullptr, "Failed to register local file for asset load path: {}", asset_ptr->load_path.string());
    }

    file_handle = fs->create_asset_virtual_file(asset_ptr->virtual_path.string());
    if (type == file_type::LOCAL) {
      file_handle->set_absolute_path(asset_ptr->absolute_path);
    } else {
      file_handle->set_absolute_path(asset_ptr->virtual_path);
    }
    file_handle->set_virtual_path(asset_ptr->virtual_path.string());

    OTHER_ASSERT(file_handle != nullptr, "Failed to create file handle for asset: {}", asset_ptr->virtual_path.string());
    CORE_LOG_DEBUG("Asset file registered for asset [{}] of type [{}] :\n{}", asset_ptr->id, asset_ptr->asset_type, file_handle->to_string());
    dir_handle->add_file(file_handle);
  }

  void asset_handler::unregister_asset_in_filesystem(const asset* asset_ptr) {
    OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null in unregister_asset_in_filesystem");
    PROFILE_SECTION("asset_handler::unregister_asset_in_filesystem");

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
    PROFILE_SECTION("asset_handler::get_asset_state_by_path_hash");
    for (const auto& [id, a] : loaded_assets) {
      if (a.path_hash == path_hash) {
        return get_asset_state(id);
      }
    }

    for (const auto& ctx : asset_pipelines) {
      if (ctx.second.loading_asset.path_hash == path_hash) {
        return get_asset_state(ctx.second.loading_asset.id);
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

  ostd::vector<natural_t> asset_handler::get_all_tracked_ids() const {
    PROFILE_SECTION("asset_handler::get_all_tracked_ids");
    ostd::vector<natural_t> ids;
    ids.reserve(loaded_assets.size() + asset_pipelines.size());

    for (const auto& [id, a] : loaded_assets) {
      ids.push_back(id);
    }

    for (const auto& ctx : asset_pipelines) {
      ids.push_back(ctx.second.loading_asset.id);
    }

    return ids;
  }

  asset_handler::pipeline_context* asset_handler::get_asset_pipeline_context(natural_t asset_id) {
    PROFILE_SECTION("asset_handler::get_asset_pipeline_context");
    for (auto& ctx : asset_pipelines) {
      if (ctx.second.loading_asset.id == asset_id) {
        return &ctx.second;
      }
    }
    return nullptr;
  }

  void asset_handler::execute_plan() {
    PROFILE_SECTION("asset_handler::execute_plan");
    plan.remaining_children.assign(snapshot.nodes.size(), 0);
    plan.failed.assign(snapshot.nodes.size(), false);
    plan.pending.assign(snapshot.nodes.size(), false);

    for (const auto& [parent, child] : snapshot.edges) {
      const dependency_snapshot::node& c = snapshot.nodes[child];
      const natural_t cid = runtime_id(c.stable_id);
      if (cid == 0 || !asset_loaded(cid) || needs_refresh.contains(c.stable_id)) {
        ++plan.remaining_children[parent];
        plan.pending[child] = true;
      }
    }

    for (uint32_t slot = 0; slot < snapshot.nodes.size(); ++slot) {
      if (plan.remaining_children[slot] == 0) {
        dispatch_slot(slot);
      }
    }
  }

  void asset_handler::dispatch_slot(uint32_t slot) {
    OTHER_ASSERT(slot < snapshot.nodes.size(), "dispatch_slot out of range: {}", slot);
    PROFILE_SECTION("asset_handler::dispatch_slot");
    if (plan.failed[slot]) {
      return;
    }

    const dependency_snapshot::node& n = snapshot.nodes[slot];
    const natural_t id = runtime_id(n.stable_id);
    if (id != 0 && !asset_loaded(id) && asset_states.contains(id)) {
      return;
    }

    if (id != 0 && asset_loaded(id)) {
      if (needs_refresh.erase(n.stable_id) > 0) {
        reload_asset(id);
      }
      return;
    }

    load_asset(absolute_of(n.virtual_path));
  }

  void asset_handler::on_planned_child_loaded(natural_t stable_id) {
    PROFILE_SECTION("asset_handler::on_planned_child_loaded");
    if (snapshot.nodes.empty()) {
      return;
    }

    const dependency_snapshot::node* n = snapshot.find(stable_id);
    if (n == nullptr) {
      return;
    }

    const uint32_t slot = detail::slot_of(snapshot, stable_id);
    if (slot >= plan.pending.size() || !plan.pending[slot]) {
      return;
    }
    plan.pending[slot] = false;

    for (const uint32_t parent : snapshot.reverse[slot]) {
      OTHER_ASSERT(plan.remaining_children[parent] > 0, "indegree underflow for '{}'", snapshot.nodes[parent].virtual_path);
      if (--plan.remaining_children[parent] == 0) {
        dispatch_slot(parent);
      }
    }
  }

  void asset_handler::on_planned_child_failed(natural_t stable_id, const std::string_view error_msg) {
    PROFILE_SECTION("asset_handler::on_planned_child_failed");
    if (snapshot.nodes.empty()) {
      return;
    }

    const dependency_snapshot::node* n = snapshot.find(stable_id);
    if (n == nullptr) {
      return;
    }

    /// plan bookkeeping must not be initiator-blind: a failed flat reload of a snapshot
    //  member would otherwise poison a plan that never dispatched it
    const uint32_t slot = detail::slot_of(snapshot, stable_id);
    if (slot >= plan.pending.size() || !plan.pending[slot]) {
      return;
    }
    plan.failed[slot] = true;
    plan.pending[slot] = false;

    ostd::vector<uint32_t> worklist{ slot };
    while (!worklist.empty()) {
      const uint32_t failed_slot = worklist.back();
      worklist.pop_back();
      for (const uint32_t parent : snapshot.reverse[failed_slot]) {
        if (plan.failed[parent]) {
          continue;
        }

        plan.failed[parent] = true;
        const dependency_snapshot::node& p = snapshot.nodes[parent];
        const std::string chain = std::format("{} failed: {}", p.virtual_path, error_msg);
        CORE_LOG_ERROR("{}", chain);

        events.trigger_event(get_asset_event_name(p.type, "asset-load-failed"), runtime_id(p.stable_id));
        worklist.push_back(parent);
      }
    }
  }

}  // namespace other