/**
 * \file dotnet/host.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_HOST_HPP
#define OTHER_SCRIPTING_DOTNET_HOST_HPP

#include <map>

#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

#include "core/config_table.hpp"
#include "core/defines.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/interop_interface.hpp"
#include "dotnet/type_cache.hpp"
#include "script/script_object.hpp"

#include "interop_interface.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <ShlObj_core.h>
  #include <Windows.h>
  #define OTHER_ENVIRONMENT_DOTNET_CALLTYPE __cdecl
  #define OTHER_ENVIRONMENT_DOTNET_HOSTFXR_NAME "hostfxr.dll"

  #ifdef _WCHAR_T_DEFINED
    #define OTHER_ENVIRONMENT_DOTNET_WIDE_CHARS
    #define DNET_STR(s) L##s
  #else
    #define DNET_STR(s) s
  #endif  // _WCHAR_T_DEFINED
#endif

#define OTHER_ENVIRONMENT_DOTNET_TARGET_VERSION_MAJOR 9
#define OTHER_ENVIRONMENT_DOTNET_TARGET_VERSION_MAJOR_STR '9'
#define OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION UNMANAGEDCALLERSONLY_METHOD

namespace other {

  class dotnet_host {
   public:
    struct clr_functions {
      hostfxr_set_error_writer_fn set_error_writer = nullptr;
      hostfxr_initialize_for_dotnet_command_line_fn init_host_cmd_line = nullptr;
      hostfxr_initialize_for_runtime_config_fn init_host_config = nullptr;
      hostfxr_get_runtime_delegate_fn get_runtime_delegate = nullptr;
      hostfxr_close_fn close_host_fxr = nullptr;
      load_assembly_and_get_function_pointer_fn get_managed_function_ptr = nullptr;
    };
    struct interop_table {
      /// AssemblyLoader
      create_assembly_load_context create_assembly_load_context = nullptr;
      unload_assembly_load_context unload_assembly_load_context = nullptr;
      load_managed_assembly load_managed_assembly = nullptr;
      unload_managed_assembly unload_managed_assembly = nullptr;
      get_last_load_status get_last_load_status = nullptr;
      get_assembly_name get_assembly_name = nullptr;

      /// NativeFunctionManager
      discover_binding_points discover_binding_points = nullptr;
      bind_native_function bind_native_function = nullptr;
      register_internal_call register_internal_call = nullptr;
      validate_binding_points validate_binding_points = nullptr;

      /// NativeObjectManager
      attach_native_object attach_native_object = nullptr;
      detach_native_object detach_native_object = nullptr;

      /// TypeInterface
      get_type_information get_assembly_types = nullptr;
      get_net_core_types get_net_core_types = nullptr;
      get_type_id get_type_id = nullptr;
      get_dotnet_type_name get_full_type_name = nullptr;

      get_type_information get_type_methods = nullptr;
      get_type_information get_type_fields = nullptr;
      get_type_information get_type_properties = nullptr;
      get_type_information get_attributes = nullptr;
      check_type_characteristic has_attribute = nullptr;

      is_derived_from derived_from = nullptr;

      //        method
      has_method has_method = nullptr;
      get_method_name get_method_name = nullptr;
      get_method_return_type get_method_return_type = nullptr;
      get_method_accessibility get_method_accessibility = nullptr;
      get_type_information get_method_parameter_types = nullptr;
      get_type_information get_method_attributes = nullptr;
      is_method_static is_method_static = nullptr;

      //       field
      field_property_checker has_field = nullptr;
      get_field_name get_field_name = nullptr;
      get_field_type get_field_type = nullptr;
      get_field_value_type get_field_value_type = nullptr;
      get_field_accessibility get_field_accessibility = nullptr;
      get_field_attributes get_field_attributes = nullptr;
      get_default_value get_default_value = nullptr;

      //       property
      get_property_name get_property_name = nullptr;
      get_property_type get_property_type = nullptr;
      get_property_attributes get_property_attributes = nullptr;

      //       attribute
      get_attribute_type get_attribute_type = nullptr;
      get_managed_object_from_object get_attribute_object = nullptr;

      /// ManagedObject
      create_object create_object = nullptr;
      destroy_object destroy_object = nullptr;
      invoke_method invoke_instance_method = nullptr;
      invoke_method_ret invoke_instance_method_ret = nullptr;
      invoke_static_method invoke_static_method = nullptr;
      invoke_static_method_ret invoke_static_method_ret = nullptr;
      field_is_private_checker is_field_private = nullptr;
      field_setter_getter set_field = nullptr;
      field_setter_getter get_field = nullptr;
      field_setter_getter set_property = nullptr;
      field_setter_getter get_property = nullptr;

      string_field_setter_getter set_string_field = nullptr;
      string_field_setter_getter get_string_field = nullptr;
      string_field_setter_getter set_string_property = nullptr;
      string_field_setter_getter get_string_property = nullptr;

      managed_strlen get_string_field_length = nullptr;
      managed_strlen get_string_property_length = nullptr;

      /// GarbageCollector
      collect_garbage collect_garbage = nullptr;
      wait_for_pending_finalizers wait_for_pending_finalizers = nullptr;

      /// BehaviorInterface
      behavior_add add_behavior = nullptr;
      behavior_remove remove_behavior = nullptr;
      behavior_remove_all remove_all_behaviors = nullptr;
      behavior_has has_behavior = nullptr;
      behavior_get_count get_behavior_count = nullptr;
      behavior_get_type_names get_behavior_type_names = nullptr;
      behavior_destroy_handle destroy_behavior_handle = nullptr;
    };

    dotnet_host();
    ~dotnet_host() = default;

#if 0
    void load_host_runtime_config(const filepath& runtime_config_path);
    void load_host_command_line(const filepath& command_line_path);
#else
    void load_host(const config_table& env_config);
#endif
    void unload_host();

    void call_entry_point();
    void rediscover_binding_points();

    assembly_context* create_assembly_context(const std::string_view name);
    void destroy_assembly_context(natural_t context_id);

    int32_t get_behavior_base_type_id();
    void purge_dotnet_type(int32_t dotnet_type_id);

    template <typename... Args>
    dotnet_object* instantiate_managed_object(const std::string_view type_name, const std::string_view name, Args&&... args) {
      dotnet_type* type = get_type_cache()->get_type(type_name);
      if (type == nullptr) {
        CORE_LOG_ERROR("Failed to find .NET type: {}", type_name);
        return nullptr;
      }
      CORE_LOG_DEBUG("Instantiating managed object of type [{}] with name [{}]", type_name, name);

      dotnet_object* res = nullptr;
      constexpr size_t argc = sizeof...(args);

      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        res = instantiate_managed_object_of_type(name, type, argv, arg_ts, argc);
      } else {
        res = instantiate_managed_object_of_type(name, type, nullptr, nullptr, 0);
      }

      return res;
    }

    dotnet_object* instantiate_managed_object_of_type(const std::string_view name, dotnet_type* type, const void** argv, const managed_type* arg_ts, size_t argc);
    void destroy_managed_object(dotnet_object* obj);

    interop_table& interop() {
      return interop_functions;
    }
    type_cache* get_type_cache() {
      return &loaded_types;
    }

    template <typename R = void, typename... Args>
    R invoke_static(const std::string_view type_name, const std::string_view method_name, Args&&... args) {
      if constexpr (std::is_same_v<R, void>) {
        invoke_static_void(type_name, method_name, std::forward<Args>(args)...);
      } else {
        return invoke_static_ret_impl<R, Args...>(type_name, method_name, std::forward<Args>(args)...);
      }
    }

   private:
    void* hostfxr_lib = nullptr;
    clr_functions coreclr;

    interop_table interop_functions;
    type_cache loaded_types;

    config_table environment_config;
    std::basic_string<char_t> dotnet_binding_assembly;
    std::basic_string<char_t> dotnet_runtime_config;

    ostd::map<natural_t, assembly_context> assembly_contexts;
    ostd::map<natural_t, dotnet_object> managed_objects;

    opt<int32_t> behavior_base_type_id = std::nullopt;

    ostd::map<natural_t, dotnet_object>::iterator destroy_managed_object(ostd::map<natural_t, dotnet_object>::iterator obj_itr);

    filepath get_bindings_assembly_path() const;

    dotnet_object* new_object(const std::string_view name, dotnet_type* type);
    ostd::map<natural_t, dotnet_object>::iterator remove_object(const std::string_view name);

    void bind_interop_table();
    void bind_native_functions();

    void* load_managed_function(const filepath& asm_path, const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type = OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION) const;

    /// \todo fix hardcoded path and replace with install location
    template <typename Fn>
    Fn load_managed_function(const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type = OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION) const {
      PROFILE_SECTION("dotnet_host::load-managed-function--by-type-and-method");
      const char_t* dotnetlib_path = dotnet_binding_assembly.data();
      return (Fn)(load_managed_function(dotnetlib_path, type_name, method_name, delegate_type));
    }

    void invoke_static_method_with_args(const std::string_view type_name, const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc);
    void invoke_static_returning_method_args(const std::string_view type_name, const std::string_view method_name, const void** argv, const managed_type* arg_ts, size_t argc, void* out);

    template <typename... Args>
    void invoke_static_void(const std::string_view type_name, const std::string_view method_name, Args&&... args) {
      constexpr size_t argc = sizeof...(args);
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        invoke_static_method_with_args(type_name, method_name, argv, arg_ts, argc);
      } else {
        invoke_static_method_with_args(type_name, method_name, nullptr, nullptr, 0);
      }
    }

    template <typename R, typename... Args>
    R invoke_static_ret_impl(const std::string_view type_name, const std::string_view method_name, Args&&... args) {
      if constexpr (std::is_pointer_v<R> || std::is_trivial_v<R>) {
        return invoke_static_trivial_pointer_ret<R, Args...>(type_name, method_name, std::forward<Args>(args)...);
      } else if constexpr (is_stringlike_type<R>) {
        return invoke_static_stringlike_ret<R, Args...>(type_name, method_name, std::forward<Args>(args)...);
      } else {
        static_assert(false, "Unsupported return type for invoke_static_ret");
      }
    }

    template <typename R, typename... Args>
    R invoke_static_trivial_pointer_ret(const std::string_view type_name, const std::string_view method_name, Args&&... args) {
      constexpr size_t argc = sizeof...(args);
      R ret{};
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        if constexpr (std::is_pointer_v<R>) {
          invoke_static_returning_method_args(type_name, method_name, argv, arg_ts, argc, (void*)ret);
        } else {
          invoke_static_returning_method_args(type_name, method_name, argv, arg_ts, argc, &ret);
        }
      } else {
        if constexpr (std::is_pointer_v<R>) {
          invoke_static_returning_method_args(type_name, method_name, nullptr, nullptr, 0, (void*)ret);
        } else {
          invoke_static_returning_method_args(type_name, method_name, nullptr, nullptr, 0, &ret);
        }
      }
      return std::move(ret);
    }

    template <typename R, typename... Args>
    R invoke_static_stringlike_ret(const std::string_view type_name, const std::string_view method_name, Args&&... args) {
      native_string ret_str = native_string::new_str("");
      constexpr size_t argc = sizeof...(args);
      if constexpr (argc > 0) {
        const void* argv[argc] = {};
        managed_type arg_ts[argc] = {};
        detail::create_opaque_handle_array<Args...>(argv, arg_ts, std::forward<Args>(args)..., std::make_index_sequence<argc>{});
        invoke_static_returning_method_args(type_name, method_name, argv, arg_ts, argc, &ret_str);
      } else {
        invoke_static_returning_method_args(type_name, method_name, nullptr, nullptr, 0, &ret_str);
      }

      std::string ret = ret_str;
      native_string::free_str(ret_str);
      return ret;
    }
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_HOST_HPP