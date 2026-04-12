/**
 * \file driver/driver_kernel.cpp
 **/
#include "driver/driver_kernel.hpp"

#include <ranges>

#include "core/logger.hpp"

namespace other {

  void driver_kernel::initialize() {
    CORE_LOG_INFO("Initializing driver kernel.");

    for (const auto type : system_order) {
      if (builtin_systems[static_cast<size_t>(type)] != nullptr) {
        builtin_systems[static_cast<size_t>(type)]->initialize(this);
      }
    }
  }

  void driver_kernel::tick(float dt) {
    for (const auto type : system_order) {
      if (builtin_systems[static_cast<size_t>(type)] != nullptr && builtin_systems[static_cast<size_t>(type)]->active()) {
        builtin_systems[static_cast<size_t>(type)]->tick(this, dt);
      }
    }

    tick_plugins(dt);
  }

  void driver_kernel::shutdown() {
    CORE_LOG_INFO("Shutting down driver kernel.");
    shutdown_plugins();

    for (auto itr = system_order.rbegin(); itr != system_order.rend(); ++itr) {
      if (builtin_systems[static_cast<size_t>(*itr)] != nullptr) {
        builtin_systems[static_cast<size_t>(*itr)]->shutdown(this);
        arena_allocator<driver_system>{}.free(builtin_systems[static_cast<size_t>(*itr)]);
        builtin_systems[static_cast<size_t>(*itr)] = nullptr;
      }
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

  driver_system* driver_kernel::install_plugin(driver_system* plugin) {
    OTHER_ASSERT(plugin != nullptr, "Cannot install null plugin.");

    auto already_installed_plugins =
      plugin_systems | std::views::keys |
      std::views::filter([&](const system_key& key) { return key.type <= driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_END && key.type >= driver_system_type::CUSTOM_DRIVER_SYSTEM_ID_START; }) |
      std::ranges::to<std::vector>();

    if (std::ranges::size(already_installed_plugins) >= kNumCustomSystemSlots) {
      CORE_LOG_ERROR("Failed to install plugin of type {}. Maximum number of plugins already installed.", kNumCustomSystemSlots);
      return nullptr;
    }

    uint32_t type = std::ranges::size(already_installed_plugins) + kCustomSystemIdStart;
    plugin->force_override_id(type);

    system_key key{
      plugin->id(),
      std::ranges::size(already_installed_plugins),
    };
    auto [itr, inserted] = plugin_systems.insert({ key, plugin });
    OTHER_ASSERT(inserted, "Failed to insert plugin into plugin systems map");

    itr->second->initialize(this);
    return itr->second;
  }

  void driver_kernel::remove_plugin(uint32_t id, size_t index) {
    system_key key{ id, index };
    if (auto itr = plugin_systems.find(key); itr != plugin_systems.end()) {
      remove_plugin(itr->second);
      plugin_systems.erase(itr);
    } else {
      CORE_LOG_ERROR("Failed to find plugin with type {} and index {} for removal.", id, index);
    }
  }

  void driver_kernel::remove_plugin(const std::string_view name) {
    auto itr = std::ranges::find_if(plugin_systems, [&](const auto& pair) { return pair.second->name() == name; });
    if (itr == plugin_systems.end()) {
      CORE_LOG_ERROR("Failed to find plugin with name '{}' for removal.", name);
      return;
    }
    remove_plugin(itr->second);
    plugin_systems.erase(itr);
  }

  void driver_kernel::remove_plugin(driver_system* plugin) {
    OTHER_ASSERT(plugin != nullptr, "Cannot remove null plugin.");
    if (plugin->active()) {
      plugin->shutdown(this);
    }
    arena_allocator<driver_system>{}.free(plugin);
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

  void driver_kernel::tick_plugins(float dt) {
    for (auto& [key, plugin] : plugin_systems) {
      if (plugin->active()) {
        plugin->tick(this, dt);
      }
    }
  }

  void driver_kernel::shutdown_plugins() {
    for (auto& [key, plugin] : plugin_systems) {
      if (plugin->active()) {
        plugin->shutdown(this);
      }
      arena_allocator<driver_system>{}.free(plugin);
    }
    plugin_systems.clear();
  }

}  // namespace other