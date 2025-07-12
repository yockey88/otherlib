/**
 * \file dotnet/host.hpp
 **/
#ifndef OTHER_SCRIPTING_DOTNET_HOST_HPP
#define OTHER_SCRIPTING_DOTNET_HOST_HPP

#include <filesystem>

#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>
// #include <dotnet/nethost.h>

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
    dotnet_host() = default;
    ~dotnet_host() = default;

#if 0
    void load_host_runtime_config(const std::filesystem::path& runtime_config_path);
    void load_host_command_line(const std::filesystem::path& command_line_path);
#else
    void load_host();
#endif

    void call_entry_point();

    void unload_host();

   private:
    clr_functions coreclr;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_DOTNET_HOST_HPP