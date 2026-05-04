/**
 * \file scripting/interface_registry.cpp
 **/
#include "scripting/interface_registry.hpp"

#include "core/fnv.hpp"

#include "plugin/plugin.hpp"

namespace other {

  natural_t interface_registry::register_interface(const environment_interface& env_interface) {
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
    OTHER_ASSERT(inserted, "Failed to register interface binding for interface '{}'.", interface_name);

    CORE_LOG_DEBUG(" - rigistered binding {}", new_binding_id);
    bind_lua_interface_methods(it->second, itr->second);

    return new_binding_id;
  }

  bool interface_registry::has_interface_method(const std::string_view interface_name, const std::string_view method_name) const {
    auto itr = std::ranges::find_if(interfaces, [&interface_name](const auto& pair) { return pair.second.name == interface_name; });
    if (itr == interfaces.end()) {
      return false;
    }
    return std::ranges::any_of(itr->second.actions, [&method_name](const auto& m) { return m.name == method_name; });
  }

  void interface_registry::invoke_interface_method(const std::string_view interface_name, const std::string_view method_name, const std::span<value> args) {
    auto itr = std::ranges::find_if(interfaces, [&interface_name](const auto& pair) {
      return pair.second.name == interface_name;
    });
    if (itr == interfaces.end()) {
      CORE_LOG_ERROR("Attempted to invoke method '{}' on unknown interface '{}'.", method_name, interface_name);
      return;
    }

    const bool method_defined = std::ranges::any_of(itr->second.actions, [&method_name](const auto& m) {
      return m.name == method_name;
    });
    if (!method_defined) {
      CORE_LOG_ERROR("Attempted to invoke unknown method '{}' on interface '{}'.", method_name, interface_name);
      return;
    }

    CORE_LOG_TRACE("[INTERFACE] Invoking method '{}.{}'", interface_name, method_name);
    for (auto& [binding_id, bi] : bound_interfaces) {
      if (bi.interface_id != itr->first) {
        continue;
      }

      auto bound_method_itr = std::ranges::find_if(bi.methods, [&method_name](const auto& m) {
        return m.method_id == method_name;
      });
      if (bound_method_itr == bi.methods.end()) {
        continue;
      }

      try {
        CORE_LOG_TRACE(" - invoking binding {}", binding_id);
        bound_method_itr->method_action.execute(args);
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error invoking interface method '{}.{}': {}", interface_name, method_name, e.what());
      } catch (...) {
        CORE_LOG_ERROR("Unknown error invoking interface method '{}.{}'.", interface_name, method_name);
      }
    }
  }

  opt<value> interface_registry::invoke_interface_method_ret(natural_t interface_binding, const std::string_view interface_name, const std::string_view method_name, const std::span<value> args) {
    CORE_LOG_TRACE("[INTERFACE] Invoking method '{}.{}'", interface_name, method_name);
    auto bi_itr = bound_interfaces.find(interface_binding);
    if (bi_itr == bound_interfaces.end()) {
      CORE_LOG_ERROR("Attempted to invoke method '{}.{}' on unknown interface binding {}.", interface_name, method_name, interface_binding);
      return std::nullopt;
    }

    auto iface_itr = interfaces.find(bi_itr->second.interface_id);
    if (iface_itr == interfaces.end()) {
      CORE_LOG_ERROR("Interface binding {} is bound to unknown interface ID {}.", interface_binding, bi_itr->second.interface_id);
      return std::nullopt;
    }

    bound_interface& bi = bi_itr->second;
    if (bi.interface_id != iface_itr->first) {
      CORE_LOG_ERROR("Attempted to invoke method '{}.{}' on interface binding {} bound to a different interface.", interface_name, method_name, interface_binding);
      return std::nullopt;
    }

    auto bound_method_itr = std::ranges::find_if(bi.methods, [&method_name](const auto& m) {
      return m.method_id == method_name;
    });
    if (bound_method_itr == bi.methods.end()) {
      CORE_LOG_ERROR("Attempted to invoke unknown method '{}.{}' on interface binding {}.", interface_name, method_name, interface_binding);
      return std::nullopt;
    }

    try {
      CORE_LOG_TRACE(" - invoking binding {}", interface_binding);
      return { bound_method_itr->method_action.execute(args) };
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Error invoking interface method '{}.{}' on binding {}: {}", interface_name, method_name, interface_binding, e.what());
    } catch (...) {
      CORE_LOG_ERROR("Unknown error invoking interface method '{}.{}' on binding {}.", interface_name, method_name, interface_binding);
    }

    CORE_LOG_ERROR("No bindings found for interface '{}', method '{}'.", interface_name, method_name);
    return std::nullopt;
  }

  bool interface_registry::validate_dotnet_interface_object(const environment_interface& env_interface, const dotnet_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot validate .NET interface object for interface '{}' because the object pointer is null.", env_interface.name);
    /// \todo implement this
    return false;
  }

  bool interface_registry::validate_lua_interface_table(const environment_interface& env_interface, const sol::table& table) const {
    for (const auto& method : env_interface.actions) {
      sol::object func_obj = table[method.lua_name.value_or(method.name).data()];
      if (!func_obj.valid() || func_obj.get_type() != sol::type::function) {
        if (method.required) {
          CORE_LOG_ERROR("Lua interface table is missing required method '{}' for interface '{}'.", method.lua_name.value_or(method.name), env_interface.name);
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
    OTHER_ASSERT(bound_interface.interface_table.has_value(), "Cannot bind Lua interface methods for interface '{}' because the bound interface has no Lua table.", env_interface.name);

    sol::table& table = bound_interface.interface_table.value();
    for (const auto& method : env_interface.actions) {
      const std::string& resolved_name = method.lua_name.value_or(method.name);

      sol::object func_obj = table[resolved_name.data()];
      const bool has = func_obj.valid() && func_obj.get_type() == sol::type::function;

      if (!has && method.required) {
        OTHER_ASSERT(false, "Lua interface table is missing required method '{}' for interface '{}'.", resolved_name, env_interface.name);
      } else if (!has) {
        CORE_LOG_DEBUG(" - method '{}' not implemented", resolved_name);
      } else {
        CORE_LOG_DEBUG(" - bound method '{}'", method.name, resolved_name);
        bound_interface.methods.push_back(bound_interface_method{
          .method_id = method.name,
          .method_action = action{
            method.name,
            method.description,
            make_ref<lua_table_callback>(table, resolved_name),
          },
        });
      }
    }
  }

  void interface_registry::bind_plugin_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface) {
    return;
  }

}  // namespace other