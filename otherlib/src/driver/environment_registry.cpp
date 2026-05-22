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

    return provider_id;
  }

  void environment_registry::revoke_all_from(std::string_view plugin_name) {
  }

}  // namespace other