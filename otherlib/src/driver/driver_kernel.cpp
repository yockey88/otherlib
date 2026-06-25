/**
 * \file driver/driver_kernel.cpp
 **/
#include "driver/driver_kernel.hpp"

#include <ranges>

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "driver/driver_system.hpp"
#include "driver/subsystem_registry.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/event_driver_system.hpp"
#include "driver/systems/input_driver_system.hpp"
#include "driver/systems/job_driver_system.hpp"
#include "driver/systems/network_system.hpp"
#include "driver/systems/peer_mesh_system.hpp"
#include "driver/systems/project_system.hpp"
#include "driver/systems/rendering_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "driver/systems/scripting_system.hpp"
#include "driver/systems/vm_system.hpp"

namespace other {

  void driver_kernel::load_profile(const std::string_view profile_name) {
    /// initialize network system regardless of whether networking is enabled or not, as some subsystems depend on it and it handles the network-disabled case internally
    add_system<network_system>(driver_system_type::NETWORK_DRIVER_SYSTEM);
    add_system<job_driver_system>(driver_system_type::JOB_DRIVER_SYSTEM);
    add_system<peer_mesh_system>(driver_system_type::PEER_MESH_DRIVER_SYSTEM);

    /// always load events/input/assets
    add_system<event_driver_system>(driver_system_type::EVENT_DRIVER_SYSTEM);
    add_system<input_driver_system>(driver_system_type::INPUT_DRIVER_SYSTEM);
    add_system<asset_system>(driver_system_type::ASSET_DRIVER_SYSTEM);

    if (subsystem_registry::profile_includes_scripting(profile_name)) {
      add_system<scripting_system>(driver_system_type::SCRIPTING_DRIVER_SYSTEM);
    }
    // if (subsystem_registry::profile_includes_physics(profile_name)) {
    //   add_system<physics_system>(driver_system_type::PHYSICS_DRIVER_SYSTEM);
    // }
    if (subsystem_registry::profile_includes_rendering(profile_name)) {
      add_system<rendering_system>(driver_system_type::RENDERING_DRIVER_SYSTEM);
    }

    if (profile_name != "minimal") {
      add_system<vm_system>(driver_system_type::VM_DRIVER_SYSTEM);
      add_system<scene_system>(driver_system_type::SCENE_DRIVER_SYSTEM);
      add_system<project_system>(driver_system_type::PROJECT_DRIVER_SYSTEM);
    }

    update_order();
  }

  void driver_kernel::load_driver_plugins_from_config(driver* driver_instance) {
    const auto& configuration = driver_instance->configuration();
    const toml::node_view plugins_node = configuration.get_raw("driver.plugins");
    if (!plugins_node || !plugins_node.is_array_of_tables()) {
      CORE_LOG_DEBUG("No driver plugins specified in configuration.");
      return;
    }

    auto* plugin_tables = plugins_node.as_array();
    if (plugin_tables == nullptr) {
      CORE_LOG_ERROR("Invalid format for driver plugins in configuration. Expected an array of tables.");
      return;
    }

    struct plugin_info {
      std::string name;
      std::string path;
    };
    std::vector<plugin_info> plugins_to_load;

    for (const auto& table : *plugin_tables) {
      if (!table.is_table()) {
        CORE_LOG_ERROR("Invalid format for driver plugins in configuration. Expected an array of tables.");
        continue;
      }

      const auto* plugin_table = table.as_table();
      if (plugin_table == nullptr) {
        CORE_LOG_ERROR("Invalid format for driver plugins in configuration. Expected an array of tables.");
        continue;
      }

      auto name_itr = plugin_table->find("name");
      auto path_itr = plugin_table->find("path");
      if (name_itr == plugin_table->end() || path_itr == plugin_table->end()) {
        CORE_LOG_ERROR("Invalid format for driver plugin entry in configuration. Each plugin entry must contain 'name' and 'path' fields.");
        continue;
      }

      if (!name_itr->second.is_string() || !path_itr->second.is_string()) {
        CORE_LOG_ERROR("Invalid format for driver plugin entry in configuration. 'name' and 'path' fields must be strings.");
        continue;
      }

      std::string plugin_name = name_itr->second.as_string()->get();
      std::string plugin_path = perform_tag_replacement(path_itr->second.as_string()->get());
      plugins_to_load.push_back({ plugin_name, plugin_path });
      CORE_LOG_DEBUG("Driver plugin specified in config: '{}' at path '{}'", plugin_name, plugin_path);
    }

    for (const auto& plugin : plugins_to_load) {
      CORE_LOG_INFO("Loading driver plugin: '{}' @ {}", plugin.name, plugin.path);

      auto* lib = plugin::load_plugin_library(plugin.path);
      if (lib == nullptr) {
        CORE_LOG_ERROR("Failed to load driver plugin library: {}", plugin.path);
        continue;
      }

      filepath path = plugin.path;
      register_driver_plugin(path, lib);
    }
  }

  void driver_kernel::initialize() {
    PROFILE_SECTION("driver_kernel::initialize");
    CORE_LOG_INFO("Initializing driver kernel.");

    {
      auto& driver_reg = environment_registries[static_cast<size_t>(interface_scope::DRIVER)];
      driver_reg.registry.register_interface<driver_plugin>(
        [this](scope<driver_plugin> p) { return 0; },
        [this](natural_t id) {},
        driver_plugin_args(driver_instance),  // empty
        interface_cardinality::MULTIPLE);
    }

    {
      PROFILE_SECTION("driver_kernel::initialize--builtin_systems");
      for (const auto type : system_order) {
        OTHER_ASSERT(builtin_systems[static_cast<size_t>(type)] != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
        CORE_LOG_DEBUG("Initializing builtin system of type {} with id {}.", builtin_systems[static_cast<size_t>(type)]->name(), type);
        builtin_systems[static_cast<size_t>(type)]->initialize(this);
      }
    }
    {
      PROFILE_SECTION("driver_kernel::initialize--late_builtin_systems");
      for (const auto type : system_order) {
        OTHER_ASSERT(builtin_systems[static_cast<size_t>(type)] != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
        CORE_LOG_DEBUG("Late initializing builtin system of type {} with id {}.", builtin_systems[static_cast<size_t>(type)]->name(), type);
        builtin_systems[static_cast<size_t>(type)]->late_initialize(this);
      }
    }
  }

  void driver_kernel::tick(double dt) {
    PROFILE_SECTION("driver_kernel::tick");
    {
      PROFILE_SECTION("driver_kernel::tick--builtin_systems");
      for (const auto type : system_order) {
        OTHER_ASSERT(builtin_systems[static_cast<size_t>(type)] != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
        builtin_systems[static_cast<size_t>(type)]->tick(this, dt);
      }
    }

    {
      PROFILE_SECTION("driver_kernel::tick--plugin_systems");
      for (auto& [key, plugin] : plugin_systems) {
        OTHER_ASSERT(plugin != nullptr, "Plugin with type {} and index {} is null.", key.type, key.index);
        if (plugin->active()) {
          plugin->tick(this, dt);
        }
      }
    }
  }

  void driver_kernel::unload_project_plugins() {
    PROFILE_SECTION("driver_kernel::unload_project_plugins");
    auto& reg = environment_registries[static_cast<size_t>(interface_scope::PROJECT)];
    for (const auto& plugin_info : reg.provided_plugins) {
      reg.registry.uninstall_plugin(plugin_info.library_name);
      plugin::unload_plugin_library(plugin_info.library_name);
    }
  }

  void driver_kernel::unload_driver_plugins() {
    PROFILE_SECTION("driver_kernel::unload_driver_plugins");
    auto& reg = environment_registries[static_cast<size_t>(interface_scope::DRIVER)];
    for (const auto& plugin_info : reg.provided_plugins) {
      reg.registry.uninstall_plugin(plugin_info.library_name);
      plugin::unload_plugin_library(plugin_info.library_name);
    }
  }

  void driver_kernel::unload_plugins() {
    PROFILE_SECTION("driver_kernel::unload_plugins");
    for (auto itr = plugin_systems.begin(); itr != plugin_systems.end();) {
      OTHER_ASSERT(itr->second != nullptr, "Plugin with type {} and index {} is null.", itr->first.type, itr->first.index);
      shutdown_plugin(itr->second);
      itr = plugin_systems.erase(itr);
    }
    plugin_systems.clear();
  }

  void driver_kernel::shutdown() {
    for (auto itr = system_order.rbegin(); itr != system_order.rend(); ++itr) {
      OTHER_ASSERT(builtin_systems[static_cast<size_t>(*itr)] != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(*itr));
      CORE_LOG_DEBUG("Shutting down builtin system of type {} with id {}.", builtin_systems[static_cast<size_t>(*itr)]->name(), *itr);
      builtin_systems[static_cast<size_t>(*itr)]->shutdown(this);
      arena_allocator<driver_system>{}.free(builtin_systems[static_cast<size_t>(*itr)]);
      builtin_systems[static_cast<size_t>(*itr)] = nullptr;
    }
  }

  void driver_kernel::update_order() {
    system_order.clear();
    system_order.reserve(kNumBuiltinDriverSystems);
    for (size_t i = 0; i < kNumBuiltinDriverSystems; ++i) {
      if (builtin_systems[i] != nullptr) {
        system_order.push_back(static_cast<driver_system_type>(i));
      }
    }
    std::ranges::sort(system_order, [&](driver_system_type a, driver_system_type b) {
      auto* system_a = builtin_systems[static_cast<size_t>(a)];
      auto* system_b = builtin_systems[static_cast<size_t>(b)];
      OTHER_ASSERT(system_a != nullptr, "System of type {} is not initialized.", static_cast<uint32_t>(a));
      OTHER_ASSERT(system_b != nullptr, "System of type {} is not initialized.", static_cast<uint32_t>(b));
      return system_a->id() < system_b->id();
    });
  }

  std::string driver_kernel::list_systems() const {
    std::stringstream ss;
    ss << "Installed Driver Systems:\n";
    for (const auto type : system_order) {
      OTHER_ASSERT(builtin_systems[static_cast<size_t>(type)] != nullptr, "Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
      ss << " - " << builtin_systems[static_cast<size_t>(type)]->name() << "\n";
    }
    // for (const auto& [key, plugin] : plugin_systems) {
    //   OTHER_ASSERT(plugin != nullptr, "Plugin with type {} and index {} is null.", key.type, key.index);
    //   ss << " - " << plugin->name() << " (Plugin)\n";
    // }
    return ss.str();
  }

  void driver_kernel::register_project_plugin(const filepath& plugin_name, library_handle* plugin_library) {
    register_plugin(environment_registries[static_cast<size_t>(interface_scope::PROJECT)], plugin_name, plugin_library);
  }

  void driver_kernel::remove_system(driver_system_type type) {
    OTHER_ASSERT(type < kNumBuiltinDriverSystems, "Invalid builtin system type: {}", static_cast<uint32_t>(type));
    driver_system* system = builtin_systems[static_cast<size_t>(type)];
    if (system == nullptr) {
      CORE_LOG_ERROR("Builtin system of type {} is not initialized.", static_cast<uint32_t>(type));
      return;
    }
    system->shutdown(this);
    arena_allocator<driver_system>{}.free(system);
    builtin_systems[static_cast<size_t>(type)] = nullptr;
  }

  void driver_kernel::shutdown_plugin(driver_system* plugin) {
    if (plugin == nullptr) {
      return;
    }

    if (plugin->active()) {
      plugin->shutdown(this);
    }

    auto itr = std::ranges::find_if(plugin_systems, [&](const auto& pair) { return pair.second == plugin; });
    remove_plugin(itr->first.type, itr->first.index);
  }

  void driver_kernel::remove_plugin(uint32_t id, size_t index) {
    system_key key{ id, index };
    if (auto itr = plugin_systems.find(key); itr != plugin_systems.end()) {
      auto name_itr = plugin_name.find(key);
      if (name_itr != plugin_name.end()) {
        plugin::unload_plugin_library(name_itr->second);
        plugin_name.erase(name_itr);
        CORE_LOG_DEBUG("Successfully removed plugin with type {} and index {}.", id, index);
      } else {
        CORE_LOG_ERROR("Failed to find plugin name for plugin with type {} and index {}.", id, index);
      }
    } else {
      CORE_LOG_ERROR("Failed to find plugin with type {} and index {} for removal.", id, index);
    }
  }

  driver_plugin* driver_kernel::get_plugin(uint32_t id, size_t index) {
    system_key key{ id, index };
    auto itr = plugin_systems.find(key);
    if (itr == plugin_systems.end()) {
      CORE_LOG_ERROR("Failed to find plugin with type {} and index {}.", id, index);
      return nullptr;
    }

    driver_plugin* plugin = dynamic_cast<driver_plugin*>(itr->second);
    OTHER_ASSERT(plugin != nullptr, "Failed to cast plugin with type {} and index {} to driver_plugin.", id, index);
    return plugin;
  }

  void driver_kernel::register_plugin(plugin_registry& registry, const filepath& path, library_handle* lib) {
    std::string name = path.filename().stem().string();

    opt<symbol> manifest_symbol = lib->get_symbol(kManifestFunctionSymbolName);
    if (!manifest_symbol.has_value()) {
      CORE_LOG_ERROR("Failed to find manifest symbol '{}' in plugin '{}'", kManifestFunctionSymbolName, path.string());
      return;
    }

    symbol& sym = manifest_symbol.value();
    OTHER_ASSERT(sym.address != nullptr, "Manifest symbol '{}' is null in plugin '{}'", kManifestFunctionSymbolName, path.string());
    plugin_manifest* (*get_manifest_fn)() = sym.get_function<plugin_manifest* (*)()>();
    plugin_manifest* manifest_ptr = get_manifest_fn();
    if (manifest_ptr == nullptr) {
      CORE_LOG_ERROR("Plugin manifest is null in plugin '{}'", path.string());
      return;
    }

    auto& manifest = *manifest_ptr;
    auto id = registry.registry.install_from_manifest(name, manifest);
    registry.provided_plugins.push_back({ name, id });
    CORE_LOG_INFO("Registered driver plugin '{}' from library '{}'", name, path.string());
  }

  void driver_kernel::register_driver_plugin(const filepath& path, library_handle* lib) {
    register_plugin(environment_registries[static_cast<size_t>(interface_scope::DRIVER)], path, lib);
  }

}  // namespace other