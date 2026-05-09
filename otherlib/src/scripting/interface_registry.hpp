/**
 * \file scripting/interface_registry.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_INTERFACE_REGISTRY_HPP
#define OTHERLIB_SCRIPTING_INTERFACE_REGISTRY_HPP

#include "scripting/environment_interface.hpp"

namespace other {

  class plugin;

  class interface_registry {
   public:
    natural_t register_interface(const environment_interface& env_interface);
    natural_t register_interface_binding(const std::string_view interface_name, sol::table interface_table);

    template <typename... Args>
    void invoke(const std::string_view interface_name, const std::string_view method_name, Args&&... args) {
      if (!has_interface_method(interface_name, method_name)) {
        CORE_LOG_WARN("Attempted to invoke unknown interface method '{}.{}'.", interface_name, method_name);
        return;
      }

      invoke_interface_method<void, Args...>(interface_name, method_name, std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    R invoke(natural_t id, const std::string_view interface_name, const std::string_view method_name, Args&&... args) {
      if (!has_interface_method(interface_name, method_name)) {
        CORE_LOG_WARN("Attempted to invoke unknown interface method '{}.{}'.", interface_name, method_name);
        return R{};
      }
      if (id == 0 || bound_interfaces.find(id) == bound_interfaces.end()) {
        CORE_LOG_WARN("Attempted to invoke interface method '{}.{}' on unknown binding with id {}.", interface_name, method_name, id);
        return R{};
      }

      return invoke_method_on_binding<R, Args...>(id, interface_name, method_name, std::forward<Args>(args)...);
    }

   private:
    struct bound_interface {
      natural_t interface_id;

      opt<sol::table> interface_table;
      dotnet_object* dotnet_interface_object = nullptr;
      plugin* plugin_ptr = nullptr;

      std::vector<bound_interface_method> methods;
    };

    natural_t interface_id_counter = 0;
    natural_t interface_binding_count = 0;

    inline natural_t generate_interface_id() { return ++interface_id_counter; }
    inline natural_t generate_interface_binding_id() { return ++interface_binding_count; }

    std::map<natural_t, environment_interface> interfaces;
    std::map<natural_t, bound_interface> bound_interfaces;

    bool has_interface_method(const std::string_view interface_name, const std::string_view method_name) const;

    template <typename R, typename... Args>
    R invoke_interface_method(const std::string_view interface_name, const std::string_view method_name, Args&&... args) {
      CORE_LOG_TRACE("[INTERFACE] Attempting to invoke '{}.{}'", interface_name, method_name);
      auto iface_itr = std::ranges::find_if(interfaces, [&interface_name](const auto& pair) {
        return pair.second.name == interface_name;
      });
      OTHER_ASSERT(iface_itr != interfaces.end(), "Interface '{}' not found when attempting to invoke method '{}'.", interface_name, method_name);

      auto method_itr = std::ranges::find_if(iface_itr->second.actions, [&method_name](const auto& m) {
        return m.name == method_name;
      });
      OTHER_ASSERT(method_itr != iface_itr->second.actions.end(), "Method '{}.{}' not found when attempting to invoke.", interface_name, method_name);

      for (auto& [binding_id, bi] : bound_interfaces) {
        auto bound_method_itr = std::ranges::find_if(bi.methods, [&method_name](const auto& m) {
          return m.method_id == method_name;
        });
        if (bound_method_itr == bi.methods.end()) {
          continue;
        }

        try {
          CORE_LOG_TRACE(" [METHOD] {}.{} (binding: {})", interface_name, method_name, binding_id);
          bound_method_itr->method_action.template execute<R, Args...>(std::forward<Args>(args)...);
        } catch (const std::exception& e) {
          CORE_LOG_ERROR("Error invoking interface method '{}.{}': {}", interface_name, method_name, e.what());
        } catch (...) {
          CORE_LOG_ERROR("Unknown error invoking interface method '{}.{}'", interface_name, method_name);
        }
      }

      return action::default_return<R>();
    }

    template <typename R, typename... Args>
    R invoke_method_on_binding(natural_t id, const std::string_view interface_name, const std::string_view method_name, Args&&... args) {
      CORE_LOG_TRACE("[INTERFACE] Attempting to invoke '{}.{}' on binding {}", interface_name, method_name, id);
      auto bi_itr = bound_interfaces.find(id);
      OTHER_ASSERT(bi_itr != bound_interfaces.end(), "Binding with id {} not found when attempting to invoke '{}.{}'.", id, interface_name, method_name);

      auto& bi = bi_itr->second;
      auto method_itr = std::ranges::find_if(bi.methods, [&method_name](const auto& m) {
        return m.method_id == method_name;
      });
      OTHER_ASSERT(method_itr != bi.methods.end(), "Method '{}.{}' not found on binding with id {} when attempting to invoke.", interface_name, method_name, id);

      try {
        CORE_LOG_TRACE(" [METHOD] {}.{} (binding: {})", interface_name, method_name, id);
        return method_itr->method_action.template execute<R, Args...>(std::forward<Args>(args)...);
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Error invoking interface method '{}.{}' on binding with id {}: {}", interface_name, method_name, id, e.what());
        return action::default_return<R>();
      } catch (...) {
        CORE_LOG_ERROR("Unknown error invoking interface method '{}.{}' on binding with id {}.", interface_name, method_name, id);
        return action::default_return<R>();
      }
    }

    bool validate_dotnet_interface_object(const environment_interface& env_interface, const dotnet_object* obj) const;
    bool validate_lua_interface_table(const environment_interface& env_interface, const sol::table& table) const;
    bool validate_plugin_interface(const environment_interface& env_interface, const plugin* plugin_instance) const;

    void bind_dotnet_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
    void bind_lua_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
    void bind_plugin_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_INTERFACE_REGISTRY_HPP