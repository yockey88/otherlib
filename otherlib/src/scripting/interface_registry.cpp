/**
 * \file scripting/interface_registry.cpp
 **/
#include "scripting/interface_registry.hpp"

#include "core/fnv.hpp"

#include "plugin/plugin.hpp"

namespace other {

  natural_t interface_registry::register_interface(const environment_interface& env_interface) {
    PROFILE_SECTION("interface_registry::register_interface");
    natural_t new_id = generate_interface_id();
    auto [it, inserted] = interfaces.emplace(new_id, env_interface);
    OTHER_ASSERT(inserted, "Failed to register environment interface '{}'.", env_interface.name);

    CORE_LOG_TRACE("[INTERFACE] '{}' (ID: {})", env_interface.name, new_id);
    for (const auto& method : env_interface.actions) {
      CORE_LOG_TRACE(" - {}: {}", method.name, method.description);
    }
    return new_id;
  }

  natural_t interface_registry::register_interface_binding(const std::string_view interface_name, sol::table interface_table) {
    PROFILE_SECTION("interface_registry::register_interface_binding");
    auto itr = std::find_if(interfaces.begin(), interfaces.end(), [&interface_name](const auto& pair) {
      return pair.second.name == interface_name;
    });
    if (itr == interfaces.end()) {
      CORE_LOG_ERROR("Attempted to register interface binding for unknown interface '{}'.", interface_name);
      return 0;
    }
    if (!validate_lua_interface_table(itr->second, interface_table)) {
      CORE_LOG_ERROR("Failed to validate Lua interface table for interface '{}'.", interface_name);
      return 0;
    }

    CORE_LOG_DEBUG("[INTERFACE] Registering [LUA] interface '{}'", interface_name);
    natural_t new_binding_id = generate_interface_binding_id();
    auto [it, inserted] = bound_interfaces.emplace(new_binding_id, bound_interface{
                                                                     .interface_id = itr->first,
                                                                     .interface_table = std::move(interface_table),
                                                                   });
    OTHER_ASSERT(inserted, "Failed to register interface binding to interface '{}'.", interface_name);

    CORE_LOG_DEBUG("[INTERFACE BINDING: {}] new {}", new_binding_id, interface_name);
    bind_lua_interface_methods(it->second, itr->second);

    return new_binding_id;
  }

  natural_t interface_registry::register_named_callback(const std::string_view callback_name, ref<callback> callback_ref) {
    natural_t new_id = generate_callback_id();
    auto [it, inserted] = bound_callbacks.emplace(FNV(callback_name), bound_callback{ .name = std::string{ callback_name }, .callback_ref = std::move(callback_ref) });
    OTHER_ASSERT(inserted, "Failed to register named callback '{}'.", callback_name);
    CORE_LOG_DEBUG("[CALLBACK] Registered named callback '{}' with ID {}", callback_name, new_id);
    return new_id;
  }

  void interface_registry::unregister_named_callback(const std::string_view callback_name) {
    auto itr = bound_callbacks.find(FNV(callback_name));
    OTHER_ASSERT(itr != bound_callbacks.end(), "Attempted to unregister unknown named callback '{}'.", callback_name);
    CORE_LOG_DEBUG("[CALLBACK] Unregistered named callback '{}'", callback_name);
    bound_callbacks.erase(itr);
  }

  bool interface_registry::has_interface_method(const std::string_view interface_name, const std::string_view method_name) const {
    auto itr = std::ranges::find_if(interfaces, [&interface_name](const auto& pair) { return pair.second.name == interface_name; });
    if (itr == interfaces.end()) {
      return false;
    }
    return std::ranges::any_of(itr->second.actions, [&method_name](const auto& m) { return m.name == method_name; });
  }

  bool interface_registry::validate_dotnet_interface_object(const environment_interface& env_interface, const dotnet_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot validate .NET interface object for interface '{}' because the object pointer is null.", env_interface.name);
    /// \todo implement this
    return false;
  }

  bool interface_registry::validate_lua_interface_table(const environment_interface& env_interface, const sol::table& table) const {
    PROFILE_SECTION("interface_registry::validate_lua_interface_table");
    for (const auto& method : env_interface.actions) {
      sol::object func_obj = table[method.script_name.value_or(method.name).data()];
      if (!func_obj.valid() || func_obj.get_type() != sol::type::function) {
        if (method.required) {
          CORE_LOG_ERROR("Lua interface table is missing required method '{}' for interface '{}'.", method.script_name.value_or(method.name), env_interface.name);
          return false;
        }
      }
    }

    return true;
  }

  bool interface_registry::validate_plugin_interface(const environment_interface& env_interface, const plugin* plugin_instance) const {
    OTHER_ASSERT(plugin_instance != nullptr, "Cannot validate plugin interface for interface '{}' because the plugin pointer is null.", env_interface.name);
    /// \todo implement this
    return false;
  }

  void interface_registry::bind_dotnet_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface) {
    return;
  }

  void interface_registry::bind_lua_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface) {
    OTHER_ASSERT(bound_interface.interface_table.has_value(), "Cannot bind Lua interface methods to interface '{}' because the bound interface has no Lua table.", env_interface.name);
    PROFILE_SECTION("interface_registry::bind_lua_interface_methods");

    sol::table& table = bound_interface.interface_table.value();
    for (const auto& method : env_interface.actions) {
      const std::string& resolved_name = method.script_name.value_or(method.name);

      sol::object func_obj = table[resolved_name.data()];
      const bool has = func_obj.valid() && func_obj.get_type() == sol::type::function;

      if (!has && method.required) {
        OTHER_ASSERT(false, "Lua interface table is missing required method '{}' to interface '{}'.", resolved_name, env_interface.name);
      } else if (!has) {
        CORE_LOG_DEBUG(" - method '{}' not implemented", resolved_name);
      } else {
        CORE_LOG_DEBUG(" - bound method '{}'", method.name, resolved_name);
        bound_interface.methods.push_back(bound_interface_method{
          .method_id = method.name,
          .method_action = action{
            method.name,
            method.description,
            make_ref<lua_callback>(table, resolved_name),
          },
        });
      }
    }
  }

  void interface_registry::bind_plugin_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface) {
    return;
  }

}  // namespace other