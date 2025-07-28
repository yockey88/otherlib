/**
 * \file dotnet/host.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_HOST_HPP
#define OTHER_SCRIPTING_DOTNET_HOST_HPP

#include <filesystem>
#include <map>

#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>

#include "core/defines.hpp"

#include "dotnet/dotnet_assembly.hpp"
#include "dotnet/interop_interface.hpp"
#include "dotnet/type_cache.hpp"

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
      get_last_load_status get_last_load_status = nullptr;
      get_assembly_name get_assembly_name = nullptr;

      /// NativeFunctionManager
      register_internal_call register_internal_call = nullptr;

      /// InteropInterface
      get_assembly_types get_assembly_types = nullptr;
      get_net_core_types get_net_core_types = nullptr;
      get_type_id get_type_id = nullptr;
      get_full_type_name get_full_type_name = nullptr;

      get_type_methods get_type_methods = nullptr;
      get_type_fields get_type_fields = nullptr;
      get_type_properties get_type_properties = nullptr;
      has_attribute has_attribute = nullptr;
      get_attributes get_attributes = nullptr;
    };

    dotnet_host();
    ~dotnet_host() = default;

#if 0
    void load_host_runtime_config(const std::filesystem::path& runtime_config_path);
    void load_host_command_line(const std::filesystem::path& command_line_path);
#else
    void load_host();
#endif
    void unload_host();

    void call_entry_point();

    assembly_context* create_assembly_context(const std::string_view name);
    void destroy_assembly_context(natural_t context_id);

    interop_table& interop() {
      return interop_functions;
    }
    type_cache* get_type_cache() {
      return &loaded_types;
    }

   private:
    void* hostfxr_lib = nullptr;
    clr_functions coreclr;

    interop_table interop_functions;
    type_cache loaded_types;

    std::map<natural_t, assembly_context> assembly_contexts;

    void bind_interop_table();
    void bind_native_functions();

    void* load_managed_function(const std::filesystem::path& asm_path, const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type = OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION) const;

    /// \todo fix hardcoded path
    template <typename Fn>
    Fn load_managed_function(const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type = OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION) const {
      const char_t* dotnetlib_path = DNET_STR("build/other-csharp/Debug/OtherCsBindings.dll");
      return (Fn)(load_managed_function(dotnetlib_path, type_name, method_name, delegate_type));
    }
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_HOST_HPP