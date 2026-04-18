/**
 * \file driver/systems/asset_system.cpp
 **/
#include "driver/systems/asset_system.hpp"

#include "file/filesystem.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/driver_mounts.hpp"
#include "driver/systems/network_system.hpp"

namespace other {

  void asset_system::initialize(driver_kernel* kernel) {
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system initialization.");

    fs->initialize_directory_structure({
      driver_mounts::kAssetMount,
      driver_mounts::kSceneMount,
      driver_mounts::kScriptMount,
    });

    event_system& events = *get_driver().get_event_system();
    auto& network = kernel->get_core_system<network_system>();
    asset_mgr = make_scope<asset_handler>(events, network.io_context(), driver_mounts::kAssetMount);

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

    events.register_event("ls-driver-default");
    events.register_event("ls-driver-windows");
    events.register_event("ls-driver-files");
    events.register_event("ls-driver-scenes");
    events.register_event("ls-driver-assets");
  }

  void asset_system::tick(driver_kernel* kernel, double dt) {
    asset_mgr->update_pipelines();

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system tick.");
    fs->poll_files();
  }

  void asset_system::shutdown(driver_kernel* kernel) {
    asset_mgr->purge_stores();
    asset_mgr = nullptr;

    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system shutdown.");
    fs->shutdown_file_system();
  }

  natural_t asset_system::begin_asset_load(const filepath& asset_path, std::function<void(natural_t)> on_loaded) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    CORE_LOG_DEBUG("Loading asset at path: {}", asset_path.string());

    natural_t asset_id = asset_mgr->load_asset(asset_path, [this, asset_path](asset* asset_ptr) {
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null.");

      auto it = std::ranges::find_if(loading_asset_ids, [asset_ptr](const auto& entry) {
        return entry.asset_id == asset_ptr->id;
      });
      OTHER_ASSERT(it != loading_asset_ids.end(), "Loading asset ID not found in tracking list.");
      CORE_LOG_DEBUG("Asset loaded callback for asset ID: {} @ path: {} (virtual path: {})", asset_ptr->id, asset_path.string(), asset_ptr->virtual_path);

      loading_asset_ids.erase(it);
      // get_driver().get_event_system()->trigger_event("assets.new-asset-loaded", asset_ptr->id);
    });

    loading_asset_ids.push_back({
      .asset_id = asset_id,
      .on_loaded = on_loaded,
    });

    return asset_id;
  }

  natural_t asset_system::add_model_source_asset(const std::string& name, const std::vector<vertex>& vertices, const std::vector<index>& indices) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_model_source_asset(name, vertices, indices);
  }

  natural_t asset_system::add_scene_asset(scene* scene_ptr, opt<filepath> scene_path) {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->add_scene_asset(scene_ptr, scene_path);
  }

  void asset_system::begin_full_unload() {
    asset_mgr->purge_stores();
  }

  void handle_ls_event(driver_kernel* kernel, const value& data);
  void handle_ls_files_event(driver_kernel* kernel, const value& data);
  void handle_ls_windows_event(driver_kernel* kernel, const value& data);
  void handle_ls_scenes_event(driver_kernel* kernel, const value& data);
  void handle_ls_assets_event(driver_kernel* kernel, const value& data);

  scope<asset_handler>& asset_system::get_asset_manager() {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr;
  }

  natural_t asset_system::get_asset_hash(natural_t asset_id) const {
    OTHER_ASSERT(asset_mgr != nullptr, "Asset manager is not initialized in driver.");
    return asset_mgr->get_asset_hash(asset_id);
  }

  void asset_system::handle_ls_event(driver_kernel* kernel, const value& data) {
    auto* fs = subsystem<file_system>::get();
    OTHER_ASSERT(fs != nullptr, "File system subsystem is not available in asset system.");

    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    const auto& mounts = fs->get_all_mounts();
    const auto& files = fs->get_all_files();

    for (auto& [hash, mount] : mounts) {
      std::stringstream ss;
      mount->print(ss);
      events->trigger_event("console.output", ss.str());
    }
    for (auto& [hash, file] : files) {
      std::stringstream ss;
      file->print(ss, 1);
      events->trigger_event("console.output", ss.str());
    }
  }

  void asset_system::handle_ls_windows_event(driver_kernel* kernel, const value& data) {
    auto& driver_ui_ptr = get_driver().get_ui();
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");

    std::vector<std::string> open_windows = driver_ui_ptr->get_open_window_names();
    std::vector<std::string> windows = std::span<const std::string_view>(driver_ui::kBuiltinWindowNames.data(), driver_ui::NUM_BUILTIN_WINDOW_TYPES).subspan(1) |
      std::views::transform([](const std::string_view& name) { return std::string(name); }) |
      std::views::filter([&open_windows](const std::string& name) { return std::ranges::find(open_windows, name) == open_windows.end(); }) |
      std::ranges::to<std::vector>();

    std::stringstream ss;
    ss << "Available Driver UI Windows:\n";
    for (const auto& window_name : open_windows) {
      ss << "  - " << window_name << " (open)\n";
    }
    for (const auto& window_name : windows) {
      ss << "  - " << window_name << "\n";
    }
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
    events->trigger_event("console.output", ss.str());
  }

  void asset_system::handle_ls_files_event(driver_kernel* kernel, const value& data) {
  }

  void asset_system::handle_ls_scenes_event(driver_kernel* kernel, const value& data) {
  }

  void asset_system::handle_ls_assets_event(driver_kernel* kernel, const value& data) {
  }

}  // namespace other