/**
 * \file dotnet/host.cpp
 **/
#include "dotnet/host.hpp"

#include <filesystem>
#include <optional>

#include "core/arena.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "serialization/reflection.hpp"

#include "script/scripting_environment.hpp"

#include "dotnet/interop_interface.hpp"
#include "dotnet/native_string.hpp"

#include "bindings/native_logger.hpp"

namespace other {
  namespace {

    std::optional<std::filesystem::path> GetHostPath();

    template <typename Fn>
    Fn load_function(void* handle, const char* name) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      auto fn = (Fn)GetProcAddress((HMODULE)handle, name);
#else
      auto fn = (Fn)dlsym(handle, name);
#endif  // OTHER_ENVIRONMENT_WINDOWS

      if (fn == nullptr) {
        CORE_LOG_ERROR("Failed to load function '{}' from hostfxr library: {}", name, std::strerror(errno));
        return nullptr;
      }
      return fn;
    }

  }  // namespace

  namespace bindings {

    struct argv {
      void* arena_native_handle = nullptr;
      void* logger_native_handle = nullptr;
      void* type_database_native_handle = nullptr;
      void* scripting_environment_native_handle = nullptr;
    };

  }  // namespace bindings

  dotnet_host::dotnet_host() {
  }

  void dotnet_host::load_host() {
    if (hostfxr_lib == nullptr) {
      std::optional<std::filesystem::path> host_path = GetHostPath();
      OTHER_ASSERT(host_path.has_value(), "Failed to find hostfxr library. Please ensure .NET SDK is installed and the path is correct.");
      CORE_LOG_INFO("Found hostfxr library at: {}", host_path->string());

      hostfxr_lib = LoadLibraryW(host_path->wstring().c_str());
      OTHER_ASSERT(hostfxr_lib != nullptr, "Failed to load hostfxr library: {}", host_path->string());
    }

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
    int32_t rc = coreclr.init_host_config(DNET_STR("other-csharp/resources/OtherCsBindings.runtimeconfig.json"), nullptr, &host_fxr);
    OTHER_ASSERT(rc == 0 && host_fxr != nullptr, "Failed to initialize hostfxr with runtime config : error code [{} : {:#08x}]", rc, rc);

    void* delegate = nullptr;
    rc = coreclr.get_runtime_delegate(host_fxr, hdt_load_assembly_and_get_function_pointer, &delegate);
    coreclr.get_managed_function_ptr = (load_assembly_and_get_function_pointer_fn)delegate;
    OTHER_ASSERT(rc == 0 && coreclr.get_managed_function_ptr != nullptr, "Failed to get managed function pointer!");

    coreclr.close_host_fxr(host_fxr);
    coreclr.close_host_fxr = nullptr;
    host_fxr = nullptr;
  }

  void dotnet_host::unload_host() {
    coreclr.init_host_cmd_line = nullptr;
    coreclr.init_host_config = nullptr;
    coreclr.get_runtime_delegate = nullptr;
    coreclr.close_host_fxr = nullptr;
    coreclr.set_error_writer = nullptr;
    coreclr.get_managed_function_ptr = nullptr;
  }

  void dotnet_host::call_entry_point() {
    const char_t* dotnet_type = DNET_STR("OtherCsBindings.Host, OtherCsBindings");
    const char_t* dotnet_type_method = DNET_STR("Entry");
    OTHER_ASSERT(std::filesystem::exists("build/other-csharp/Debug/OtherCsBindings.dll"), "Managed assembly not found: build/other-csharp/Debug/OtherCsBindings.dll");

    bind_interop_table();
    bind_native_functions();

    using entry_point_t = void(OTHER_ENVIRONMENT_DOTNET_CALLTYPE*)(bindings::argv);
    entry_point_t entry_point = load_managed_function<entry_point_t>(dotnet_type, dotnet_type_method);
    OTHER_ASSERT(entry_point != nullptr, "Failed to load managed entry point function: {}.{}", detail::convert_string(dotnet_type), detail::convert_string(dotnet_type_method));

    bindings::argv args{
      .arena_native_handle = subsystem<arena>::get(),
      .logger_native_handle = subsystem<logger>::get(),
      .type_database_native_handle = subsystem<type_database>::get(),
      .scripting_environment_native_handle = subsystem<scripting_environment>::get()
    };
    entry_point(args);
  }

  assembly_context* dotnet_host::create_assembly_context(const std::string_view name) {
    OTHER_ASSERT(!name.empty(), "Assembly context name cannot be empty.");
    int32_t context_handle = -1;
    {
      native_scoped_string name_str = native_string::new_str(name);
      context_handle = interop_functions.create_assembly_load_context(name_str);
    }

    natural_t context_id = FNV(name);
    {
      auto itr = assembly_contexts.find(context_id);
      if (itr != assembly_contexts.end()) {
        CORE_LOG_ERROR("Assembly context with ID {} already exists.", context_id);
        return nullptr;
      }
    }

    auto [itr, inserted] = assembly_contexts.insert({ context_id, assembly_context{ name, FNV(name), this } });
    OTHER_ASSERT(inserted, "Failed to insert assembly context into map");
    itr->second.dotnet_id = context_handle;

    CORE_LOG_INFO("Created assembly context [{}:{}]", itr->second.get_handle(), itr->second.get_name());
    return &itr->second;
  }

  void dotnet_host::destroy_assembly_context(natural_t context_id) {
    auto itr = assembly_contexts.find(context_id);
    if (itr != assembly_contexts.end()) {
      CORE_LOG_INFO("Destroying assembly context [{}:{}]", itr->second.get_handle(), itr->second.get_name());

      // interop_functions.collect_garbage(0, dotother::GCMode::DEFAULT, true, true);
      // interop_functions.wait_for_pending_finalizers();

      interop_functions.unload_assembly_load_context(itr->second.dotnet_id);
      itr->second.dotnet_id = -1;
      // itr->second.assemblies.clear();

      assembly_contexts.erase(itr);
      CORE_LOG_INFO("Destroyed assembly context with ID {}", context_id);
    } else {
      CORE_LOG_ERROR("Failed to destroy assembly context: ID {} not found", context_id);
    }
  }

  void dotnet_host::bind_interop_table() {
    CORE_LOG_DEBUG("Binding interop table...");

    const char_t* assembly_loader_type_str = DNET_STR("OtherCsBindings.AssemblyLoader, OtherCsBindings");
    const char_t* native_function_manager_type_str = DNET_STR("OtherCsBindings.NativeFunctionManager, OtherCsBindings");
    const char_t* interop_interface_type_str = DNET_STR("OtherCsBindings.InteropInterface, OtherCsBindings");

    /// AssemblyLoader
    interop_functions.create_assembly_load_context = load_managed_function<create_assembly_load_context>(assembly_loader_type_str, DNET_STR("CreateAssemblyLoadContext"));
    OTHER_ASSERT(interop_functions.create_assembly_load_context != nullptr, "Failed to load CreateAssemblyLoadContext function from managed assembly.");

    interop_functions.unload_assembly_load_context = load_managed_function<unload_assembly_load_context>(assembly_loader_type_str, DNET_STR("UnloadAssemblyLoadContext"));
    OTHER_ASSERT(interop_functions.unload_assembly_load_context != nullptr, "Failed to load UnloadAssemblyLoadContext function from managed assembly.");

    interop_functions.get_last_load_status = load_managed_function<get_last_load_status>(assembly_loader_type_str, DNET_STR("GetLastLoadStatus"));
    OTHER_ASSERT(interop_functions.get_last_load_status != nullptr, "Failed to load GetLastLoadStatus function from managed assembly.");

    interop_functions.load_managed_assembly = load_managed_function<load_managed_assembly>(assembly_loader_type_str, DNET_STR("LoadManagedAssembly"));
    OTHER_ASSERT(interop_functions.load_managed_assembly != nullptr, "Failed to load LoadManagedAssembly function from managed assembly.");

    interop_functions.get_assembly_name = load_managed_function<get_assembly_name>(assembly_loader_type_str, DNET_STR("GetAssemblyName"));
    OTHER_ASSERT(interop_functions.get_assembly_name != nullptr, "Failed to load GetAssemblyName function from managed assembly.");

    /// NativeFunctionManager
    interop_functions.register_internal_call = load_managed_function<register_internal_call>(native_function_manager_type_str, DNET_STR("RegisterInternalCall"));
    OTHER_ASSERT(interop_functions.register_internal_call != nullptr, "Failed to load RegisterInternalCall function from managed assembly.");

    /// InteropInterface
    interop_functions.get_assembly_types = load_managed_function<get_assembly_types>(interop_interface_type_str, DNET_STR("GetAssemblyTypes"));
    OTHER_ASSERT(interop_functions.get_assembly_types != nullptr, "Failed to load GetAssemblyTypes function from managed assembly.");

    interop_functions.get_net_core_types = load_managed_function<get_net_core_types>(interop_interface_type_str, DNET_STR("GetNetCoreTypes"));
    OTHER_ASSERT(interop_functions.get_net_core_types != nullptr, "Failed to load GetNetCoreTypes function from managed assembly.");

    interop_functions.get_type_id = load_managed_function<get_type_id>(interop_interface_type_str, DNET_STR("GetTypeId"));
    OTHER_ASSERT(interop_functions.get_type_id != nullptr, "Failed to load function from managed assembly.");

    interop_functions.get_full_type_name = load_managed_function<get_full_type_name>(interop_interface_type_str, DNET_STR("GetFullTypeName"));
    OTHER_ASSERT(interop_functions.get_full_type_name != nullptr, "Failed to load function from managed assembly.");

    interop_functions.get_type_methods = load_managed_function<get_type_methods>(interop_interface_type_str, DNET_STR("GetTypeMethods"));
    OTHER_ASSERT(interop_functions.get_type_methods != nullptr, "Failed to load GetTypeMethods from managed assembly.");

    interop_functions.get_type_fields = load_managed_function<get_type_fields>(interop_interface_type_str, DNET_STR("GetTypeFields"));
    OTHER_ASSERT(interop_functions.get_type_fields != nullptr, "Failed to load GetTypeFields from managed assembly.");

    interop_functions.get_type_properties = load_managed_function<get_type_properties>(interop_interface_type_str, DNET_STR("GetTypeProperties"));
    OTHER_ASSERT(interop_functions.get_type_properties != nullptr, "Failed to load GetTypeProperties from managed assembly.");

    interop_functions.has_attribute = load_managed_function<has_attribute>(interop_interface_type_str, DNET_STR("HasAttribute"));
    OTHER_ASSERT(interop_functions.has_attribute != nullptr, "Failed to load HasAttribute from managed assembly.");

    interop_functions.get_attributes = load_managed_function<get_attributes>(interop_interface_type_str, DNET_STR("GetAttributes"));
    OTHER_ASSERT(interop_functions.get_attributes != nullptr, "Failed to load GetAttributes from managed assembly.");
  }

  void dotnet_host::bind_native_functions() {
    {
      native_scoped_string function_name = native_string::new_str("OtherCsBindings.Logger+NativeLogMessage, OtherCsBindings");
      interop_functions.register_internal_call(function_name, (void*)&bindings::native_log_message);
    }
  }

  void* dotnet_host::load_managed_function(const std::filesystem::path& asm_path, const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type) const {
    if (!std::filesystem::exists(asm_path)) {
      CORE_LOG_ERROR("Assembly {} does not exist!", asm_path.string());
      return nullptr;
    }

    void* handle = nullptr;
    int32_t rc = coreclr.get_managed_function_ptr(asm_path.c_str(), type_name.c_str(), method_name.c_str(), delegate_type, nullptr, &handle);
    if (rc == -1 || handle == nullptr) {
      CORE_LOG_ERROR("Failed to load managed function {} ({}) | error code : {}", detail::convert_string(method_name), detail::convert_string(type_name), rc);
      return nullptr;
    } else {
      CORE_LOG_TRACE("Loaded managed function {} ({}) | handle: {}", detail::convert_string(method_name), detail::convert_string(type_name), handle);
    }

    return handle;
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