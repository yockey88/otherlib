/**
 * \file driver/systems/asset_system.cpp
 **/
#include "driver/systems/asset_system.hpp"

#include "file/filesystem.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/driver_mounts.hpp"
#include "driver/systems/job_driver_system.hpp"
#include "driver/systems/network_system.hpp"

namespace other {

  void asset_system::initialize(driver_kernel* kernel) {
    PROFILE_SECTION("asset_system::initialize");
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system initialization.");

    mount_mounts(kernel);

    event_system& events = *get_driver().get_event_system();

    events.register_event("ls.files");
    events.add_listener("ls.files", [this](const value& data) { handle_ls_event(&get_driver().get_kernel(), data); });
    events.register_event("ls.assets");
    events.add_listener("ls.assets", [this](const value& data) { handle_ls_assets_event(&get_driver().get_kernel(), data); });

    events.register_event("assets.all-assets-unloaded");
    events.add_listener("assets.all-assets-unloaded", [this](const value& data) {
      get_driver().confirm_assets_clean();
    });

    auto register_asset_events = [&events](asset::type asset_type) {
      events.register_event(other::get_asset_event_name(asset_type, "asset-loaded"));
      events.register_event(other::get_asset_event_name(asset_type, "asset-unloaded"));
      events.register_event(other::get_asset_event_name(asset_type, "asset-load-failed"));
      events.register_event(other::get_asset_event_name(asset_type, "asset-unload-failed"));
    };
    register_asset_events(asset::TEXTURE);
    register_asset_events(asset::MODEL_SOURCE);
    register_asset_events(asset::MODEL);
    register_asset_events(asset::ANIMATION);
    register_asset_events(asset::SCRIPT_PROJECT);
    register_asset_events(asset::SCRIPT_SOURCE);
    register_asset_events(asset::SCRIPT_FILE);
    register_asset_events(asset::SCRIPT);
    register_asset_events(asset::AUDIO);
    register_asset_events(asset::SCENE);
    register_asset_events(asset::INPUT_MAP);
    register_asset_events(asset::RENDERING_PIPELINE);
  }

  void asset_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("asset_system::tick");
    asset_mgr->update_pipelines();
    if (asset_mgr->all_assets_unloaded()) {
      get_driver().get_event_system()->trigger_event("assets.all-assets-unloaded");
    }

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system tick.");
    fs->poll_files();
  }

  void asset_system::shutdown(driver_kernel* kernel) {
    PROFILE_SECTION("asset_system::shutdown");
    asset_mgr = nullptr;

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system shutdown.");
    fs->shutdown_file_system();
  }

  void asset_system::resolve_and_load_roots(std::span<const filepath> roots) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    PROFILE_SECTION("asset_system::resolve_and_load_roots");

    for (const filepath& root : roots) {
      OTHER_ASSERT(std::filesystem::exists(root), "Resolve root '{}' does not exist.", root.string());
      CORE_LOG_DEBUG("Resolving asset root: {}", root.string());
    }
    asset_mgr->resolve_roots(roots);
    push_watch_filters();
  }

  void asset_system::file_changed(const filepath& path) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    PROFILE_SECTION("asset_system::file_changed");

    asset_mgr->re_resolve(path);
    push_watch_filters();
  }

  natural_t asset_system::begin_asset_load(const filepath& asset_path) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    PROFILE_SECTION("asset_system::begin_asset_load");

    CORE_LOG_DEBUG("Loading asset at path: {}", asset_path.string());

    natural_t asset_id = asset_mgr->load_asset(asset_path);
    if (asset_id == 0) {
      CORE_LOG_ERROR("Failed to begin asset load for path: {}", asset_path.string());
      return 0;
    }
    return asset_id;
  }

  void asset_system::begin_asset_unload(natural_t asset_id) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    CORE_LOG_DEBUG("Beginning asset unload for asset ID: {}", asset_id);
    asset_mgr->unload_asset(asset_id);
  }

  void asset_system::reload_asset(natural_t asset_id) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    CORE_LOG_DEBUG("Reloading asset for asset ID: {}", asset_id);

    auto* ass = asset_mgr->get_asset(asset_id);
    if (ass == nullptr) {
      CORE_LOG_ERROR("Cannot reload asset for asset ID: {} because it is not currently loaded.", asset_id);
      return;
    }

    if (asset_mgr->in_snapshot(ass->stable_id)) {
      asset_mgr->re_resolve(ass->load_path);
      push_watch_filters();
    } else {
      asset_mgr->reload_asset(asset_id);
    }
  }

  void asset_system::push_watch_filters() {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available while pushing watch filters.");

    /// domain glob_sets are replace-registered on every re-parse, so the pointers must be
    //  re-pushed after every resolve/re_resolve — stale filters would let bin/ churn through
    for (const manifest_domain& d : asset_mgr->manifest_domains()) {
      fs->apply_watch_filter(d.root_abs, &d.set);
    }
  }

  natural_t asset_system::add_model_source_asset(const std::string& name, const std::span<const vertex> vertices, const std::span<const index> indices) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_model_source_asset(name, vertices, indices);
  }

  natural_t asset_system::add_scene_asset(scene* scene_ptr, opt<filepath> scene_path) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_scene_asset(scene_ptr, scene_path);
  }

  natural_t asset_system::add_rendering_pipeline_asset(const std::string_view name, const pipeline_definition& definition) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_rendering_pipeline_asset(name, definition);
  }

  void asset_system::begin_full_unload() {
    CORE_LOG_DEBUG("Beginning full unload of all assets.");
    asset_mgr->begin_unload();
  }

  asset* asset_system::get_asset(natural_t asset_id) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset(asset_id);
  }

  asset* asset_system::get_asset_by_virtual_path(const filepath& virtual_path) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_by_virtual_path(virtual_path);
  }

  asset_handler::pipeline_context* asset_system::get_asset_pipeline_context(natural_t asset_id) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_pipeline_context(asset_id);
  }

  scope<asset_handler>& asset_system::get_asset_manager() {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr;
  }

  natural_t asset_system::get_asset_hash(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_hash(asset_id);
  }

  // this is to be nice for the VM who wants to treat things as 64 bit values
  natural_t asset_system::get_asset_state(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return static_cast<natural_t>(asset_mgr->get_asset_state(asset_id));
  }

  natural_t asset_system::get_asset_id_from_path(const filepath& path) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_id_from_path(path);
  }

  opt<filepath> asset_system::get_local_asset_path(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_local_asset_path(asset_id);
  }

  opt<filepath> asset_system::get_virtual_asset_path(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_virtual_asset_path(asset_id);
  }

  ostd::vector<asset*> asset_system::get_assets_of_type(asset::type type) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_assets_of_type(type);
  }

  void asset_system::mount_mounts(driver_kernel* kernel) {
    PROFILE_SECTION("asset_system::mount_mounts");
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system is null while mounting asset mounts!");

    const ostd::vector<std::string_view> default_mounts{
      driver_mounts::kAssetMount,
      driver_mounts::kSceneMount,
      driver_mounts::kScriptMount,
    };
    fs->initialize_directory_structure(default_mounts);

    event_system& events = *get_driver().get_event_system();
    auto& jobs = sibling<job_driver_system>(*kernel).get_job_system();
    asset_mgr = make_scope<asset_handler>(events, jobs, driver_mounts::kAssetMount);

    CORE_LOG_DEBUG("Configuring filesystem mounts from configuration");
    const auto md_mnts = get_driver().configuration().get_raw("filesystem.mounts");
    if (md_mnts) {
      if (md_mnts.is_array_of_tables()) {
        const auto* mounts_tables = md_mnts.as_array();
        OTHER_ASSERT(mounts_tables != nullptr, "Invalid format for filesystem mounts in configuration. Expected an array of tables.");

        CORE_LOG_DEBUG("Found {} filesystem mount entries in configuration", mounts_tables->size());
        for (const auto& table : *mounts_tables) {
          OTHER_ASSERT(table.is_table(), "Invalid format for filesystem mounts in configuration. Expected an array of tables.");
          const auto* mount_table = table.as_table();
          OTHER_ASSERT(mount_table != nullptr, "Invalid format for filesystem mounts in configuration. Expected an array of tables.");

          auto name_itr = mount_table->find("name");
          auto type_itr = mount_table->find("type");
          auto path_itr = mount_table->find("path");
          if (name_itr == mount_table->end() || type_itr == mount_table->end()) {
            CORE_LOG_ERROR("Invalid format for filesystem mount entry in configuration. Each mount entry must contain 'name' and 'type' fields.");
            continue;
          }
          if (!name_itr->second.is_string() || !type_itr->second.is_string()) {
            CORE_LOG_ERROR("Invalid format for filesystem mount entry in configuration. 'name' and 'type' fields must be strings.");
            continue;
          }

          std::string name = name_itr->second.as_string()->get();
          std::string type = type_itr->second.as_string()->get();
          /// physical is probably going to be the default use case and needs extra checking
          if (type == "physical") {
            if (path_itr == mount_table->end()) {
              CORE_LOG_ERROR("Invalid format for physical filesystem mount entry in configuration. Physical mounts must contain a 'path' field.");
              continue;
            }
            if (!path_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for physical filesystem mount entry in configuration. 'path' field must be a string.");
              continue;
            }
            std::string path_str = path_itr->second.as_string()->get();
            filepath path(path_str);
            if (!std::filesystem::exists(path)) {
              CORE_LOG_ERROR("Filesystem mount path '{}' does not exist. Cannot configure filesystem mount '{}'.", path_str, name);
              continue;
            }

            fs->mount_directory(name, path);
          }
          /// virtual is simpler
          else if (type == "virtual") {
            if (fs->is_mounted(name)) {
              CORE_LOG_WARN("Filesystem mount '{}' is already mounted. Skipping virtual mount.", name);
              continue;
            }

            fs->mount_virtual(name);
          } else {
            CORE_LOG_ERROR("Invalid filesystem mount type '{}' for mount '{}'. Supported types are 'physical' and 'virtual'.", type, name);
          }
        }
      } else {
        CORE_LOG_ERROR("Invalid format for filesystem mounts in configuration. Expected an array of tables.");
      }
    }
  }

  void asset_system::handle_ls_event(driver_kernel* kernel, const value& data) {
    PROFILE_SECTION("asset_system::handle_ls_event");
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system.");

    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    const auto& mounts = fs->get_all_mounts();
    const auto& files = fs->get_all_files();

    std::stringstream ss;
    for (auto& [hash, mount] : mounts) {
      mount->print(ss) << "\n";
    }
    for (auto& [hash, file] : files) {
      file->print(ss) << "\n";
    }
    CORE_LOG_INFO("Filesystem Contents:\n{}", ss.str());
  }

  void asset_system::handle_ls_assets_event(driver_kernel* kernel, const value& data) {
    PROFILE_SECTION("asset_system::handle_ls_assets_event");
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    std::span<const natural_t> assets = asset_mgr->get_all_asset_ids();

    std::stringstream ss;
    for (const natural_t id : assets) {
      auto* asset = asset_mgr->get_asset(id);
      if (asset != nullptr) {
        ss << std::format("ID: {}, Type: {}, Path: {}", asset->id, asset->asset_type, asset->virtual_path.string());
        ss << std::format(" [State: {}]", asset_mgr->get_asset_state(id));
        ss << "\n";
      }
    }
    CORE_LOG_INFO("Assets:\n{}", ss.str());
  }

}  // namespace other