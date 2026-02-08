/**
 * \file dotnet/host.cpp
 **/
#include "dotnet/host.hpp"

#include <filesystem>
#include <optional>

#include "core/arena.hpp"
#include "core/config_table.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "serialization/reflection.hpp"

#include "bindings/native_logger.hpp"
#include "dotnet/interop_interface.hpp"
#include "dotnet/native_string.hpp"
#include "script/scripting_environment.hpp"

namespace other {
  namespace {

    std::optional<filepath> get_host_path();

    template <typename Fn>
    Fn load_function(void* handle, const char* name) {
      PROFILE_SECTION("other::<detail>::load_function");
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

  void dotnet_host::load_host(const config_table& env_config) {
    PROFILE_SECTION("dotnet_host::load-host");
    this->environment_config = env_config;

    std::string dotnet_binding_asm = env_config.get_value<std::string>(configuration::kDotnetBindings, "C:/OtherEnvironment/dotnet-assemblies/OtherCsBindings.dll");
    OTHER_ASSERT(!dotnet_binding_asm.empty(), "Dotnet binding assembly path cannot be empty.");
    OTHER_ASSERT(std::filesystem::exists(dotnet_binding_asm), "Dotnet binding assembly not found at path: {}", dotnet_binding_asm);
    CORE_LOG_DEBUG("Using dotnet bindings assembly at path: {}", dotnet_binding_asm);
    this->dotnet_binding_assembly = detail::convert_string(dotnet_binding_asm);

    std::string dotnet_runtime_config_path = env_config.get_value<std::string>(configuration::kDotnetRuntimeConfig, "C:/OtherEnvironment/dotnet-assemblies/OtherCsBindings.runtimeconfig.json");
    OTHER_ASSERT(!dotnet_runtime_config_path.empty(), "Dotnet runtime config path cannot be empty.");
    OTHER_ASSERT(std::filesystem::exists(dotnet_runtime_config_path), "Dotnet runtime config not found at path: {}", dotnet_runtime_config_path);
    CORE_LOG_DEBUG("Using dotnet runtime config at path: {}", dotnet_runtime_config_path);
    this->dotnet_runtime_config = detail::convert_string(dotnet_runtime_config_path);

    if (hostfxr_lib == nullptr) {
      std::optional<filepath> host_path = get_host_path();
      OTHER_ASSERT(host_path.has_value(), "Failed to find hostfxr library. Please ensure .NET SDK is installed and the path is correct.");
      CORE_LOG_DEBUG("Found hostfxr library at: {}", host_path->string());

      PROFILE_SECTION("dotnet_host::load-host--load-library");
      hostfxr_lib = LoadLibraryW(host_path->wstring().c_str());
      OTHER_ASSERT(hostfxr_lib != nullptr, "Failed to load hostfxr library: {}", host_path->string());
    }

    /// 0 - success
    /// 1 - success, already initialized
    /// 2 - success, different runtime properties
    hostfxr_handle host_fxr = nullptr;
    {
      PROFILE_SECTION("dotnet_host::load-host--load-coreclr");
      {
        PROFILE_SECTION("dotnet_host::load-host--load-functions");
        coreclr.init_host_cmd_line = load_function<hostfxr_initialize_for_dotnet_command_line_fn>(hostfxr_lib, "hostfxr_initialize_for_dotnet_command_line");
        coreclr.init_host_config = load_function<hostfxr_initialize_for_runtime_config_fn>(hostfxr_lib, "hostfxr_initialize_for_runtime_config");
        coreclr.get_runtime_delegate = load_function<hostfxr_get_runtime_delegate_fn>(hostfxr_lib, "hostfxr_get_runtime_delegate");
        // coreclr.run_app = load_function<hostfxr_run_app_fn>(hostfxr_lib, "hostfxr_run_app");
        coreclr.close_host_fxr = load_function<hostfxr_close_fn>(hostfxr_lib, "hostfxr_close");
        coreclr.set_error_writer = load_function<hostfxr_set_error_writer_fn>(hostfxr_lib, "hostfxr_set_error_writer");
      }
      OTHER_ASSERT(coreclr.init_host_cmd_line != nullptr && coreclr.init_host_config != nullptr && coreclr.get_runtime_delegate != nullptr && coreclr.close_host_fxr != nullptr, "Failed to load hostfxr functions");

      /// fix this, need to loop this up in ProgramFiles for a ddeployed build, for dev this works, but could fail the CI pipeline as it is very dependent on CWD being correct
      CORE_LOG_TRACE("Initializing hostfxr with runtime config: {}", detail::convert_string(dotnet_runtime_config));
      int32_t rc = -1;
      const char_t* path = dotnet_runtime_config.c_str();
      {
        PROFILE_SECTION("dotnet_host::load-host--init-hostfxr-config");
        rc = coreclr.init_host_config(path, nullptr, &host_fxr);
      }

      OTHER_ASSERT(host_fxr != nullptr, "Failed to initialize hostfxr with runtime config : error code [{} : {:#08x}]", rc, rc);
      if (rc < 0 || rc > 2) {
        OTHER_ASSERT(false, "Failed to initialize hostfxr with runtime config: error code [{} : {:#08x}]", rc, rc);
      }

      void* delegate = nullptr;
      {
        PROFILE_SECTION("dotnet_host::load-host--get-runtime-delegate");
        rc = coreclr.get_runtime_delegate(host_fxr, hdt_load_assembly_and_get_function_pointer, &delegate);
      }
      coreclr.get_managed_function_ptr = (load_assembly_and_get_function_pointer_fn)delegate;

      OTHER_ASSERT(rc == 0 && coreclr.get_managed_function_ptr != nullptr, "Failed to get managed function pointer!");
      {
        PROFILE_SECTION("dotnet_host::load-host--close-hostfxr");
        coreclr.close_host_fxr(host_fxr);
        coreclr.close_host_fxr = nullptr;
      }
    }
    host_fxr = nullptr;
  }

  void dotnet_host::unload_host() {
    /// gc
    if (interop_functions.collect_garbage != nullptr) {
      OTHER_ASSERT(interop_functions.wait_for_pending_finalizers != nullptr, "Interop function wait_for_pending_finalizers is not initialized");
      interop_functions.collect_garbage(0, gc_mode::DEFAULT, true, true);
      interop_functions.wait_for_pending_finalizers();
    }

    auto* type_cache = get_type_cache();
    OTHER_ASSERT(type_cache != nullptr, "Type cache is null");

    type_cache->clear_cache(this);

    coreclr.init_host_cmd_line = nullptr;
    coreclr.init_host_config = nullptr;
    coreclr.get_runtime_delegate = nullptr;
    coreclr.close_host_fxr = nullptr;
    coreclr.set_error_writer = nullptr;
    coreclr.get_managed_function_ptr = nullptr;
  }

  void dotnet_host::call_entry_point() {
    PROFILE_SECTION("dotnet_host::call-entry-point");
    const char_t* dotnet_type = DNET_STR("OtherCsBindings.Host, OtherCsBindings");
    const char_t* dotnet_type_method = DNET_STR("Entry");

    filepath managed_asm_path = get_bindings_assembly_path();
    OTHER_ASSERT(std::filesystem::exists(managed_asm_path), "Managed assembly not found: {}", managed_asm_path.string());

    bind_interop_table();
    bind_native_functions();

    using entry_point_t = void(OTHER_ENVIRONMENT_DOTNET_CALLTYPE*)(bindings::argv);
    entry_point_t entry_point = load_managed_function<entry_point_t>(dotnet_type, dotnet_type_method);
    OTHER_ASSERT(entry_point != nullptr, "Failed to load managed entry point function: {}.{}", detail::convert_string(dotnet_type), detail::convert_string(dotnet_type_method));
    {
      PROFILE_SECTION("dotnet_host::call-entry-point--invoke");
      bindings::argv args{
        .arena_native_handle = subsystem<arena>::get(),
        .logger_native_handle = subsystem<logger>::get(),
        .type_database_native_handle = subsystem<type_database>::get(),
        .scripting_environment_native_handle = subsystem<scripting_environment>::get()
      };
      entry_point(args);
    }
  }

  void dotnet_host::rediscover_binding_points() {
    PROFILE_SECTION("dotnet_host::rediscover-binding-points");
    interop().discover_binding_points();
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

    CORE_LOG_DEBUG("Created assembly context [{}:{}]", itr->second.get_handle(), itr->second.get_name());
    return &itr->second;
  }

  void dotnet_host::destroy_assembly_context(natural_t context_id) {
    auto itr = assembly_contexts.find(context_id);
    if (itr != assembly_contexts.end()) {
      CORE_LOG_DEBUG("Destroying assembly context [{}:{}]", itr->second.get_handle(), itr->second.get_name());

      // interop_functions.collect_garbage(0, dotother::GCMode::DEFAULT, true, true);
      // interop_functions.wait_for_pending_finalizers();

      interop_functions.unload_assembly_load_context(itr->second.dotnet_id);
      itr->second.dotnet_id = -1;
      // itr->second.assemblies.clear();

      assembly_contexts.erase(itr);
      CORE_LOG_DEBUG("Destroyed assembly context with ID {}", context_id);
    } else {
      CORE_LOG_ERROR("Failed to destroy assembly context: ID {} not found", context_id);
    }
  }

  dotnet_object* dotnet_host::instantiate_managed_object_of_type(const std::string_view name, dotnet_type* type, const void** argv, const managed_type* arg_ts, size_t argc) {
    OTHER_ASSERT(type != nullptr, "Type cannot be null");
    OTHER_ASSERT(type->dotnet_id != -1, "Type ID is invalid: {}", type->dotnet_id);
    OTHER_ASSERT(interop_functions.create_object != nullptr, "Interop function create_object is not initialized");

    CORE_LOG_DEBUG("Attempting to create managed object [{}] of type [{}]", name, type->full_name());
    dotnet_object* obj = new_object(name, type);
    obj->managed_object = interop_functions.create_object(type->dotnet_id, false, argv, arg_ts, argc);
    if (obj->managed_object == nullptr) {
      CORE_LOG_ERROR("Failed to create managed object of type [{}]", type->full_name());
      remove_object(name);
      return nullptr;
    } else {
      CORE_LOG_DEBUG("Created managed object [{}] of type [{}]", name, type->full_name());
    }
    return obj;
  }

  void dotnet_host::destroy_managed_object(dotnet_object* obj) {
    OTHER_ASSERT(obj != nullptr, "dotnet_object is null");
    if (obj->managed_object == nullptr) {
      CORE_LOG_ERROR("Cannot destroy object: managed_object is null");
      return;
    }

    interop_functions.destroy_object(obj->managed_object);
    obj->managed_object = nullptr;

    remove_object(obj->object_name);
  }

  filepath dotnet_host::get_bindings_assembly_path() const {
    return environment_config.get_value<std::string>("scripting.dotnet-bindings", "C:/OtherEnvironment/dotnet-assemblies/OtherCsBindings.dll");
  }

  dotnet_object* dotnet_host::new_object(const std::string_view name, dotnet_type* type) {
    OTHER_ASSERT(!name.empty(), "Object name cannot be empty.");
    OTHER_ASSERT(type != nullptr, "Type cannot be null");

    auto [itr, success] = managed_objects.emplace(FNV(name), dotnet_object{ this });
    if (!success) {
      CORE_LOG_ERROR("Failed to create new managed object: Object with name '{}' already exists.", name);
      return &managed_objects.at(FNV(name));
    }
    itr->second.object_name = name;
    itr->second.dn_type = type;
    return &itr->second;
  }

  void dotnet_host::remove_object(const std::string_view name) {
    CORE_LOG_DEBUG("Removing managed object '{}'", name);
    auto it = managed_objects.find(FNV(name));
    if (it != managed_objects.end()) {
      managed_objects.erase(it);
    } else {
      CORE_LOG_ERROR("Failed to remove managed object: Object with name '{}' not found.", name);
    }
  }

  void dotnet_host::bind_interop_table() {
    PROFILE_SECTION("dotnet_host::bind-interop-table");
    CORE_LOG_DEBUG("Binding interop table...");

    const char_t* assembly_loader_type_str = DNET_STR("OtherCsBindings.AssemblyLoader, OtherCsBindings");
    const char_t* native_function_manager_type_str = DNET_STR("OtherCsBindings.NativeFunctionManager, OtherCsBindings");
    const char_t* native_object_manager_type_str = DNET_STR("OtherCsBindings.NativeObjectManager, OtherCsBindings");
    const char_t* type_interface_type_str = DNET_STR("OtherCsBindings.TypeInterface, OtherCsBindings");
    const char_t* managed_object_type_str = DNET_STR("OtherCsBindings.ManagedObject, OtherCsBindings");
    const char_t* garbage_collector_type_str = DNET_STR("OtherCsBindings.GarbageCollector, OtherCsBindings");

    /// AssemblyLoader
    interop_functions.create_assembly_load_context = load_managed_function<create_assembly_load_context>(assembly_loader_type_str, DNET_STR("CreateAssemblyLoadContext"));
    OTHER_ASSERT(interop_functions.create_assembly_load_context != nullptr, "Failed to load CreateAssemblyLoadContext function from managed assembly.");

    interop_functions.unload_assembly_load_context = load_managed_function<unload_assembly_load_context>(assembly_loader_type_str, DNET_STR("UnloadAssemblyLoadContext"));
    OTHER_ASSERT(interop_functions.unload_assembly_load_context != nullptr, "Failed to load UnloadAssemblyLoadContext function from managed assembly.");

    interop_functions.get_last_load_status = load_managed_function<get_last_load_status>(assembly_loader_type_str, DNET_STR("GetLastLoadStatus"));
    OTHER_ASSERT(interop_functions.get_last_load_status != nullptr, "Failed to load GetLastLoadStatus function from managed assembly.");

    interop_functions.load_managed_assembly = load_managed_function<load_managed_assembly>(assembly_loader_type_str, DNET_STR("LoadManagedAssembly"));
    OTHER_ASSERT(interop_functions.load_managed_assembly != nullptr, "Failed to load LoadManagedAssembly function from managed assembly.");

    interop_functions.unload_managed_assembly = load_managed_function<unload_managed_assembly>(assembly_loader_type_str, DNET_STR("UnloadManagedAssembly"));
    OTHER_ASSERT(interop_functions.unload_managed_assembly != nullptr, "Failed to load UnloadManagedAssembly function from managed assembly.");

    interop_functions.get_assembly_name = load_managed_function<get_assembly_name>(assembly_loader_type_str, DNET_STR("GetAssemblyName"));
    OTHER_ASSERT(interop_functions.get_assembly_name != nullptr, "Failed to load GetAssemblyName function from managed assembly.");

    /// NativeFunctionManager
    interop_functions.discover_binding_points = load_managed_function<discover_binding_points>(native_function_manager_type_str, DNET_STR("RediscoverBindingPoints"));
    OTHER_ASSERT(interop_functions.discover_binding_points != nullptr, "Failed to load DiscoverBindingPoints function from managed assembly.");

    interop_functions.bind_native_function = load_managed_function<bind_native_function>(native_function_manager_type_str, DNET_STR("BindNativeFunction"));
    OTHER_ASSERT(interop_functions.bind_native_function != nullptr, "Failed to load BindNativeFunction function from managed assembly.");

    interop_functions.register_internal_call = load_managed_function<register_internal_call>(native_function_manager_type_str, DNET_STR("RegisterInternalCall"));
    OTHER_ASSERT(interop_functions.register_internal_call != nullptr, "Failed to load RegisterInternalCall function from managed assembly.");

    interop_functions.validate_binding_points = load_managed_function<validate_binding_points>(native_function_manager_type_str, DNET_STR("ValidateBindingPoints"));
    OTHER_ASSERT(interop_functions.validate_binding_points != nullptr, "Failed to load ValidateBindingPoints function from managed assembly.");

    /// NativeObjectManager
    interop_functions.attach_native_object = load_managed_function<attach_native_object>(native_object_manager_type_str, DNET_STR("AttachNativeObject"));
    OTHER_ASSERT(interop_functions.attach_native_object != nullptr, "Failed to load AttachNativeObject function from managed assembly.");

    interop_functions.detach_native_object = load_managed_function<detach_native_object>(native_object_manager_type_str, DNET_STR("DetachNativeObject"));
    OTHER_ASSERT(interop_functions.detach_native_object != nullptr, "Failed to load DetachNativeObject function from managed assembly.");

    /// TypeInterface
    interop_functions.get_assembly_types = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetAssemblyTypes"));
    OTHER_ASSERT(interop_functions.get_assembly_types != nullptr, "Failed to load GetAssemblyTypes function from managed assembly.");

    interop_functions.get_net_core_types = load_managed_function<get_net_core_types>(type_interface_type_str, DNET_STR("GetNetCoreTypes"));
    OTHER_ASSERT(interop_functions.get_net_core_types != nullptr, "Failed to load GetNetCoreTypes function from managed assembly.");

    interop_functions.get_type_id = load_managed_function<get_type_id>(type_interface_type_str, DNET_STR("GetTypeId"));
    OTHER_ASSERT(interop_functions.get_type_id != nullptr, "Failed to load function from managed assembly.");

    interop_functions.get_full_type_name = load_managed_function<get_type_name>(type_interface_type_str, DNET_STR("GetFullTypeName"));
    OTHER_ASSERT(interop_functions.get_full_type_name != nullptr, "Failed to load function from managed assembly.");

    interop_functions.get_type_methods = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetTypeMethods"));
    OTHER_ASSERT(interop_functions.get_type_methods != nullptr, "Failed to load GetTypeMethods from managed assembly.");

    interop_functions.get_type_fields = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetTypeFields"));
    OTHER_ASSERT(interop_functions.get_type_fields != nullptr, "Failed to load GetTypeFields from managed assembly.");

    interop_functions.get_type_properties = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetTypeProperties"));
    OTHER_ASSERT(interop_functions.get_type_properties != nullptr, "Failed to load GetTypeProperties from managed assembly.");

    interop_functions.get_attributes = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetAttributes"));
    OTHER_ASSERT(interop_functions.get_attributes != nullptr, "Failed to load GetAttributes from managed assembly.");

    interop_functions.has_attribute = load_managed_function<check_type_characteristic>(type_interface_type_str, DNET_STR("HasAttribute"));
    OTHER_ASSERT(interop_functions.has_attribute != nullptr, "Failed to load HasAttribute from managed assembly.");

    //        method
    interop_functions.get_method_name = load_managed_function<get_method_name>(type_interface_type_str, DNET_STR("GetMethodName"));
    OTHER_ASSERT(interop_functions.get_method_name != nullptr, "Failed to load GetMethodName from managed assembly.");

    interop_functions.get_method_return_type = load_managed_function<get_method_return_type>(type_interface_type_str, DNET_STR("GetMethodReturnType"));
    OTHER_ASSERT(interop_functions.get_method_return_type != nullptr, "Failed to load GetMethodReturnType from managed assembly.");

    interop_functions.get_method_accessibility = load_managed_function<get_method_accessibility>(type_interface_type_str, DNET_STR("GetMethodAccessibility"));
    OTHER_ASSERT(interop_functions.get_method_accessibility != nullptr, "Failed to load GetMethodAccessibility from managed assembly.");

    interop_functions.get_method_parameter_types = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetMethodParameterTypes"));
    OTHER_ASSERT(interop_functions.get_method_parameter_types != nullptr, "Failed to load GetMethodParameterTypes from managed assembly.");

    interop_functions.get_method_attributes = load_managed_function<get_type_information>(type_interface_type_str, DNET_STR("GetMethodAttributes"));
    OTHER_ASSERT(interop_functions.get_method_attributes != nullptr, "Failed to load GetMethodAttributes from managed assembly.");

    //       field
    interop_functions.has_field = load_managed_function<field_property_checker>(type_interface_type_str, DNET_STR("HasField"));
    OTHER_ASSERT(interop_functions.has_field != nullptr, "Failed to load HasField from managed assembly.");

    interop_functions.get_field_name = load_managed_function<get_field_name>(type_interface_type_str, DNET_STR("GetFieldName"));
    OTHER_ASSERT(interop_functions.get_field_name != nullptr, "Failed to load GetFieldName from managed assembly.");

    interop_functions.get_field_type = load_managed_function<get_field_type>(type_interface_type_str, DNET_STR("GetFieldType"));
    OTHER_ASSERT(interop_functions.get_field_type != nullptr, "Failed to load GetFieldType from managed assembly.");

    interop_functions.get_field_value_type = load_managed_function<get_field_value_type>(type_interface_type_str, DNET_STR("GetFieldValueType"));
    OTHER_ASSERT(interop_functions.get_field_value_type != nullptr, "Failed to load GetFieldValueType from managed assembly.");

    interop_functions.get_field_accessibility = load_managed_function<get_field_accessibility>(type_interface_type_str, DNET_STR("GetFieldAccessibility"));
    OTHER_ASSERT(interop_functions.get_field_accessibility != nullptr, "Failed to load GetFieldAccessibility from managed assembly.");

    interop_functions.get_field_attributes = load_managed_function<get_field_attributes>(type_interface_type_str, DNET_STR("GetFieldAttributes"));
    OTHER_ASSERT(interop_functions.get_field_attributes != nullptr, "Failed to load GetFieldAttributes from managed assembly.");

    interop_functions.get_default_value = load_managed_function<get_default_value>(type_interface_type_str, DNET_STR("GetDefaultValue"));
    OTHER_ASSERT(interop_functions.get_default_value != nullptr, "Failed to load GetDefaultValue from managed assembly.");

    //       property
    interop_functions.get_property_name = load_managed_function<get_property_name>(type_interface_type_str, DNET_STR("GetPropertyName"));
    OTHER_ASSERT(interop_functions.get_property_name != nullptr, "Failed to load GetPropertyName from managed assembly.");

    interop_functions.get_property_type = load_managed_function<get_property_type>(type_interface_type_str, DNET_STR("GetPropertyType"));
    OTHER_ASSERT(interop_functions.get_property_type != nullptr, "Failed to load GetPropertyType from managed assembly.");

    interop_functions.get_property_attributes = load_managed_function<get_property_attributes>(type_interface_type_str, DNET_STR("GetPropertyAttributes"));
    OTHER_ASSERT(interop_functions.get_property_attributes != nullptr, "Failed to load GetPropertyAttributes from managed assembly.");

    //       attribute
    interop_functions.get_attribute_type = load_managed_function<get_attribute_type>(type_interface_type_str, DNET_STR("GetAttributeType"));
    OTHER_ASSERT(interop_functions.get_attribute_type != nullptr, "Failed to load GetAttributeType from managed assembly.");

    interop_functions.get_attribute_object = load_managed_function<get_managed_object_from_object>(type_interface_type_str, DNET_STR("GetAttributeValue"));
    OTHER_ASSERT(interop_functions.get_attribute_object != nullptr, "Failed to load GetAttributeValue from managed assembly.");

    /// ManagedObject
    interop_functions.create_object = load_managed_function<create_object>(managed_object_type_str, DNET_STR("CreateObject"));
    OTHER_ASSERT(interop_functions.create_object != nullptr, "Failed to load CreateObject from managed assembly.");

    interop_functions.destroy_object = load_managed_function<destroy_object>(managed_object_type_str, DNET_STR("DestroyObject"));
    OTHER_ASSERT(interop_functions.destroy_object != nullptr, "Failed to load DestroyObject from managed assembly.");

    // interop_functions.invoke_static_method = load_managed_function<invoke_method>(managed_object_type_str, DNET_STR("InvokeStaticMethod"));
    // OTHER_ASSERT(interop_functions.invoke_static_method != nullptr, "Failed to load InvokeStaticMethod from managed assembly.");

    // interop_functions.invoke_static_method_ret = load_managed_function<invoke_method_ret>(managed_object_type_str, DNET_STR("InvokeStaticMethodRet"));
    // OTHER_ASSERT(interop_functions.invoke_static_method_ret != nullptr, "Failed to load InvokeStaticMethodRet from managed assembly.");

    interop_functions.invoke_method = load_managed_function<invoke_method>(managed_object_type_str, DNET_STR("InvokeMethod"));
    OTHER_ASSERT(interop_functions.invoke_method != nullptr, "Failed to load InvokeMethod from managed assembly.");

    interop_functions.invoke_method_ret = load_managed_function<invoke_method_ret>(managed_object_type_str, DNET_STR("InvokeMethodRet"));
    OTHER_ASSERT(interop_functions.invoke_method_ret != nullptr, "Failed to load InvokeMethodRet from managed assembly.");

    interop_functions.is_field_private = load_managed_function<field_is_private_checker>(managed_object_type_str, DNET_STR("IsFieldPrivate"));
    OTHER_ASSERT(interop_functions.is_field_private != nullptr, "Failed to load IsFieldPrivate from managed assembly.");

    interop_functions.set_field = load_managed_function<field_setter_getter>(managed_object_type_str, DNET_STR("SetField"));
    OTHER_ASSERT(interop_functions.set_field != nullptr, "Failed to load SetField from managed assembly.");

    interop_functions.get_field = load_managed_function<field_setter_getter>(managed_object_type_str, DNET_STR("GetField"));
    OTHER_ASSERT(interop_functions.get_field != nullptr, "Failed to load GetField from managed assembly.");

    interop_functions.set_property = load_managed_function<field_setter_getter>(managed_object_type_str, DNET_STR("SetProperty"));
    OTHER_ASSERT(interop_functions.set_property != nullptr, "Failed to load SetProperty from managed assembly.");

    interop_functions.get_property = load_managed_function<field_setter_getter>(managed_object_type_str, DNET_STR("GetProperty"));
    OTHER_ASSERT(interop_functions.get_property != nullptr, "Failed to load GetProperty from managed assembly.");

    interop_functions.set_string_field = load_managed_function<string_field_setter_getter>(managed_object_type_str, DNET_STR("SetStringField"));
    OTHER_ASSERT(interop_functions.set_string_field != nullptr, "Failed to load SetStringField from managed assembly.");

    interop_functions.get_string_field = load_managed_function<string_field_setter_getter>(managed_object_type_str, DNET_STR("GetStringField"));
    OTHER_ASSERT(interop_functions.get_string_field != nullptr, "Failed to load GetStringField from managed assembly.");

    interop_functions.set_string_property = load_managed_function<string_field_setter_getter>(managed_object_type_str, DNET_STR("SetStringProperty"));
    OTHER_ASSERT(interop_functions.set_string_property != nullptr, "Failed to load SetStringProperty from managed assembly.");

    interop_functions.get_string_property = load_managed_function<string_field_setter_getter>(managed_object_type_str, DNET_STR("GetStringProperty"));
    OTHER_ASSERT(interop_functions.get_string_property != nullptr, "Failed to load GetStringProperty from managed assembly.");

    interop_functions.get_string_field_length = load_managed_function<managed_strlen>(managed_object_type_str, DNET_STR("GetStringFieldLength"));
    OTHER_ASSERT(interop_functions.get_string_field_length != nullptr, "Failed to load GetStringFieldLength from managed assembly.");

    interop_functions.get_string_property_length = load_managed_function<managed_strlen>(managed_object_type_str, DNET_STR("GetStringPropertyLength"));
    OTHER_ASSERT(interop_functions.get_string_property_length != nullptr, "Failed to load GetStringPropertyLength from managed assembly.");

    /// GarbageCollector
    interop_functions.collect_garbage = load_managed_function<collect_garbage>(garbage_collector_type_str, DNET_STR("CollectGarbage"));
    OTHER_ASSERT(interop_functions.collect_garbage != nullptr, "Failed to load CollectGarbage from managed assembly.");

    interop_functions.wait_for_pending_finalizers = load_managed_function<wait_for_pending_finalizers>(garbage_collector_type_str, DNET_STR("WaitForPendingFinalizers"));
    OTHER_ASSERT(interop_functions.wait_for_pending_finalizers != nullptr, "Failed to load WaitForPendingFinalizers from managed assembly.");
  }

  void dotnet_host::bind_native_functions() {
    PROFILE_SECTION("dotnet_host::bind-native-functions");
    {
      /// logging has to happen first
      native_scoped_string function_name = native_string::new_str("OtherCsBindings.Logger+NativeLogMessage, OtherCsBindings");
      interop_functions.register_internal_call(function_name, (void*)&bindings::native_log_message);
    }
  }

  void* dotnet_host::load_managed_function(const filepath& asm_path, const std::basic_string<char_t>& type_name, const std::basic_string<char_t>& method_name, const char_t* delegate_type) const {
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

    std::optional<filepath> get_host_path() {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      filepath base_path = "";

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