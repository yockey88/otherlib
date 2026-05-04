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
      CORE_LOG_TRACE("[INTERFACE] attempting to invoke '{}.{}'", interface_name, method_name);
      if (!has_interface_method(interface_name, method_name)) {
        CORE_LOG_WARN("Attempted to invoke unknown interface method '{}.{}'.", interface_name, method_name);
        return;
      }

      std::vector<value> arg_objects{ std::forward<Args>(args)... };
      invoke_interface_method(interface_name, method_name, arg_objects);
    }

    template <typename... Args>
    auto invoke_ret(natural_t interface_binding, const std::string_view interface_name, const std::string_view method_name, Args&&... args) -> opt<value> {
      CORE_LOG_TRACE("[INTERFACE] attempting to invoke '{}.{}' with return value", interface_name, method_name);
      if (!has_interface_method(interface_name, method_name)) {
        CORE_LOG_WARN("Attempted to invoke unknown interface method '{}.{}'.", interface_name, method_name);
        return std::nullopt;
      }

      std::vector<value> arg_objects{ std::forward<Args>(args)... };
      return invoke_interface_method_ret(interface_binding, interface_name, method_name, arg_objects);
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
    void invoke_interface_method(const std::string_view interface_name, const std::string_view method_name, const std::span<value> args);
    opt<value> invoke_interface_method_ret(natural_t interface_binding, const std::string_view interface_name, const std::string_view method_name, const std::span<value> args);

    bool validate_dotnet_interface_object(const environment_interface& env_interface, const dotnet_object* obj) const;
    bool validate_lua_interface_table(const environment_interface& env_interface, const sol::table& table) const;
    bool validate_plugin_interface(const environment_interface& env_interface, const plugin* plugin_instance) const;

    void bind_dotnet_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
    void bind_lua_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
    void bind_plugin_interface_methods(bound_interface& bound_interface, const environment_interface& env_interface);
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_INTERFACE_REGISTRY_HPP