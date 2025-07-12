/**
 * \file dotnet/host.cpp
 **/
#include "dotnet/host.hpp"

#include <filesystem>
#include <optional>

#include "core/logger.hpp"

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
  namespace {

    std::optional<std::filesystem::path> GetHostPath();

    template <typename Fn>
    Fn load_function(void* handle, const char* name) {
#ifdef _WIN32
      auto fn = (Fn)GetProcAddress((HMODULE)handle, name);
#else   // __linux__
      auto fn = (Fn)dlsym(handle, name);
#endif  // !_WIN32
      if (fn == nullptr) {
        CORE_LOG_ERROR("Failed to load function '{}' from hostfxr library: {}", name, std::strerror(errno));
        return nullptr;
      }
      return fn;
    }

  }  // namespace

  namespace bindings {

    struct argv {
    };

  }  // namespace bindings

  void dotnet_host::load_host() {
    std::optional<std::filesystem::path> host_path = GetHostPath();
    OTHER_ASSERT(host_path.has_value(), "Failed to find hostfxr library. Please ensure .NET SDK is installed and the path is correct.");
    CORE_LOG_INFO("Found hostfxr library at: {}", host_path->string());

    void* hostfxr_lib = LoadLibraryW(host_path->wstring().c_str());
    OTHER_ASSERT(hostfxr_lib != nullptr, "Failed to load hostfxr library: {}", host_path->string());

    coreclr.init_host_cmd_line = load_function<hostfxr_initialize_for_dotnet_command_line_fn>(hostfxr_lib, "hostfxr_initialize_for_dotnet_command_line");
    coreclr.init_host_config = load_function<hostfxr_initialize_for_runtime_config_fn>(hostfxr_lib, "hostfxr_initialize_for_runtime_config");
    coreclr.get_runtime_delegate = load_function<hostfxr_get_runtime_delegate_fn>(hostfxr_lib, "hostfxr_get_runtime_delegate");
    // coreclr.run_app = load_function<hostfxr_run_app_fn>(hostfxr_lib, "hostfxr_run_app");
    coreclr.close_host_fxr = load_function<hostfxr_close_fn>(hostfxr_lib, "hostfxr_close");
    coreclr.set_error_writer = load_function<hostfxr_set_error_writer_fn>(hostfxr_lib, "hostfxr_set_error_writer");

    OTHER_ASSERT(coreclr.init_host_cmd_line != nullptr && coreclr.init_host_config != nullptr && coreclr.get_runtime_delegate != nullptr && coreclr.close_host_fxr != nullptr, "Failed to load hostfxr functions");

    /// 0 - success
    /// 1 - success, already initialized
    /// 2 - success, different runtime properties
    hostfxr_handle host_fxr = nullptr;
    int32_t rc = coreclr.init_host_config(DNET_STR("other-csharp-bindings/resources/Other.runtimeconfig.json"), nullptr, &host_fxr);
    OTHER_ASSERT(rc == 0 && host_fxr != nullptr, "Failed to initialize hostfxr with runtime config : error code [{} : {:#08x}]", rc, rc);

    void* delegate = nullptr;
    rc = coreclr.get_runtime_delegate(host_fxr, hdt_load_assembly_and_get_function_pointer, &delegate);
    coreclr.get_managed_function_ptr = (load_assembly_and_get_function_pointer_fn)delegate;
    OTHER_ASSERT(rc == 0 && coreclr.get_managed_function_ptr != nullptr, "Failed to get managed function pointer!");

    coreclr.close_host_fxr(host_fxr);
    coreclr.close_host_fxr = nullptr;
    host_fxr = nullptr;
  }

  void dotnet_host::call_entry_point() {
    const char_t* dotnetlib_path = DNET_STR("build/other-csharp-bindings/Debug/Other.dll");
    const char_t* dotnet_type = DNET_STR("Other.Host, Other");
    const char_t* dotnet_type_method = DNET_STR("Entry");
    OTHER_ASSERT(std::filesystem::exists("build/other-csharp-bindings/Debug/Other.dll"), "Managed assembly not found: build/other-csharp-bindings/Debug/Other.dll");

    void* entry_point = nullptr;
    int32_t rc = coreclr.get_managed_function_ptr(dotnetlib_path, dotnet_type, dotnet_type_method, OTHER_ENVIRONMENT_DOTNET_UNMANAGED_FUNCTION, nullptr, &entry_point);
    OTHER_ASSERT(rc == 0 && entry_point != nullptr, "Failed to load managed function pointer : error code: [{} | {:#08x}]", rc, rc);

    using entry_point_t = void(OTHER_ENVIRONMENT_DOTNET_CALLTYPE*)(bindings::argv);
    auto entry = (entry_point_t)entry_point;
    bindings::argv args{};
    entry(args);
  }

  void dotnet_host::unload_host() {
    coreclr.init_host_cmd_line = nullptr;
    coreclr.init_host_config = nullptr;
    coreclr.get_runtime_delegate = nullptr;
    coreclr.close_host_fxr = nullptr;
    coreclr.set_error_writer = nullptr;
    coreclr.get_managed_function_ptr = nullptr;
  }

  namespace {

    std::optional<std::filesystem::path> GetHostPath() {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      std::filesystem::path base_path = "";

      TCHAR buffer[MAX_PATH];
      SHGetSpecialFolderPath(nullptr, buffer, CSIDL_PROGRAM_FILES, FALSE);

      base_path = buffer;
      base_path /= "dotnet/host/fxr";
      auto search_paths = std::array{
        base_path
      };
#else
      static_assert(false, "GetHostPath is not implemented for non-Windows platforms.");

#endif  // !DOTOTHER_WINDOWS
      for (const auto& path : search_paths) {
        if (!std::filesystem::exists(path)) {
          continue;
        }

        for (const auto& dir : std::filesystem::recursive_directory_iterator(path)) {
          if (!dir.is_directory()) {
            continue;
          }

          auto dp = dir.path();
          if (dp.string().find(OTHER_ENVIRONMENT_DOTNET_TARGET_VERSION_MAJOR_STR) == std::string::npos) {
            continue;
          }

          auto res = dp / OTHER_ENVIRONMENT_DOTNET_HOSTFXR_NAME;
          if (!std::filesystem::exists(res)) {
            return std::nullopt;
          }

          return res;
        }
      }

      return std::nullopt;
    }

  }  // namespace
}  // namespace other