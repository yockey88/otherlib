/**
 * \file driver/environment_registry.cpp
 **/
#include "driver/environment_registry.hpp"

namespace other {

  natural_t environment_registry::install_from_manifest(std::string_view plugin_name, const plugin_manifest& m) {
    OTHER_ASSERT(m.factory_function != nullptr, "Plugin manifest for plugin '{}' does not have a valid factory function.", plugin_name);
    CORE_LOG_INFO("Installing plugin '{}' for interface [{}]", plugin_name, m.class_name);
    CORE_LOG_INFO(" - Plugin Instance Name: {}", m.plugin_instance_name);
    CORE_LOG_INFO(" - Interface Hash: {}", m.interface_hash);

    auto itr = std::ranges::find(registered_interfaces, m.interface_hash, &registered_interface::interface_hash);
    if (itr == registered_interfaces.end()) {
      CORE_LOG_ERROR("Failed to find registered interface with hash {} for plugin '{}'", m.interface_hash, plugin_name);
      return 0;
    }

    registered_interface& reg_interface = *itr;
    if (reg_interface.cardinality == interface_cardinality::SINGLE && !reg_interface.providers.empty()) {
      OTHER_ASSERT(reg_interface.providers.size() == 1, "Interface [{}] provided by plugin '{}' is marked as SINGLE but has multiple providers registered.", reg_interface.interface_full_name, plugin_name);
      CORE_LOG_ERROR("Interface [{}] provided by plugin '{}' does not allow multiple providers, but a provider is already registered.", reg_interface.interface_full_name, plugin_name);
      return 0;
    }
    OTHER_ASSERT(reg_interface.install_thunk != nullptr, "Registered interface [{}] does not have a valid install thunk for plugin '{}'", reg_interface.interface_full_name, plugin_name);

    natural_t provider_id = reg_interface.install_thunk(reinterpret_cast<void*>(m.factory_function), m.parameters);
    if (provider_id == 0) {
      CORE_LOG_ERROR("Failed to install plugin '{}' for interface [{}].", plugin_name, reg_interface.interface_full_name);
      return 0;
    }

    CORE_LOG_INFO("Plugin '{}' installed succesfully for interface [{}] with provider ID {}", plugin_name, reg_interface.interface_full_name, provider_id);
    reg_interface.providers.push_back({
      std::string{ plugin_name },
      provider_id,
    });
    return provider_id;
  }

  void environment_registry::uninstall_plugin(std::string_view plugin_name) {
    auto interface_itr = std::ranges::find_if(registered_interfaces, [&](const registered_interface& reg_interface) {
      return std::ranges::find(reg_interface.providers, plugin_name, &plugin_provider::plugin_name) != reg_interface.providers.end();
    });
    if (interface_itr == registered_interfaces.end()) {
      CORE_LOG_ERROR("Failed to find any registered interface provided by plugin '{}'", plugin_name);
      return;
    }

    auto provider_itr = std::ranges::find(interface_itr->providers, plugin_name, &plugin_provider::plugin_name);
    OTHER_ASSERT(provider_itr != interface_itr->providers.end(), "Failed to find plugin provider '{}' in registered interface [{}]", plugin_name, interface_itr->interface_full_name);
    OTHER_ASSERT(interface_itr->revoke_thunk != nullptr, "Registered interface [{}] does not have a valid revoke thunk for plugin '{}'", interface_itr->interface_full_name, plugin_name);
    interface_itr->revoke_thunk(provider_itr->provider_id);
    interface_itr->providers.erase(provider_itr);
  }

}  // namespace other