/**
 * \file driver/system/scripting_system.cpp
 **/
#include "driver/systems/scripting_system.hpp"

#include "thread/thread_safety.hpp"

#include "lua/lua_script.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "scripting/bindings.hpp"
#include "tools/environment_console.hpp"

namespace other {

  void scripting_system::initialize(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scripting_system::initialize");
    driver_main_lua_script = subsystem<scripting_environment>::get()->load_lua_file("driver.lua");
    if (driver_main_lua_script) {
      environment_console::initialize(driver_main_lua_script);
    } else {
      CORE_LOG_ERROR("Failed to load driver main Lua script.");
    }

    get_driver().get_event_system()->register_event("console.check-command");
    get_driver().get_event_system()->add_listener("console.check-command", [this](const value& data) {
      if (data.type() == value_type::STRING) {
        std::string command = data;
        if (driver_main_lua_script && driver_main_lua_script->has_symbol("__is_command") && driver_main_lua_script->call_function<bool>("__is_command", command)) {
          get_driver().get_event_system()->trigger_event("console.command", command);
        } else {
          get_driver().get_event_system()->trigger_event("console.output", command);
        }
      }
    });

    /// load client specific .NET
    /// \note this has to happen here because .NET can override native subsystem implementations meaning we need to load these before initializing rendering or other subsystems
    std::vector<std::string> dotnet_modules = get_driver().get_config_value<std::vector<std::string>>("scripting.dotnet-modules");
    for (const auto& module : dotnet_modules) {
      CORE_LOG_DEBUG(" - .NET module to load: {}", module);
      auto assembly = load_dotnet_module(module);
      if (assembly == nullptr) {
        CORE_LOG_ERROR("Failed to load .NET module: {}", module);
      }

      loaded_dotnet_modules.push_back(assembly);
    }

    /// bind the native driver to the scripting environment to glue user scripts to the native environment
    do_script_interface_bindings(&get_driver());

    event_system& events = *get_driver().get_event_system();
    events.add_listener("script-source.asset-loaded", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::UINT64, "Expected uint64 asset ID for script source asset-loaded event");
      uint64_t asset_id = data;

      asset* asset_ptr = get_driver().get_asset(asset_id);
      OTHER_ASSERT(asset_ptr != nullptr, "Asset pointer is null for asset ID: {}", asset_id);
      if (asset_ptr->asset_type != asset::type::SCRIPT_SOURCE) {
        CORE_LOG_ERROR("Received script-source.asset-loaded event for asset ID {} but asset type is not SCRIPT_SOURCE", asset_id);
        return;
      }

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");

      ref<assembly> asm_ref = env->get_dotnet_module_by_asset_path(asset_ptr->absolute_path);
      OTHER_ASSERT(asm_ref != nullptr, "Failed to load .NET assembly for script asset: {}", asset_ptr->absolute_path.string());

      ostd::vector<callback_binding> bindings = asm_ref->get_native_function_bindings();
      CORE_LOG_DEBUG("Found {} native callback bindings in assembly [{}:{}]", bindings.size(), asm_ref->get_handle(), asm_ref->get_name());
      for (const auto& binding : bindings) {
        auto last_dot = binding.full_type_and_method_name.find_last_of('.');
        std::string type_name = binding.full_type_and_method_name.substr(0, last_dot);
        std::string method_name = binding.full_type_and_method_name.substr(last_dot + 1);
        CORE_LOG_INFO("Native Callback Binding registered: {} -> {}.{}", binding.binding_name, type_name, method_name);
        get_driver().get_interface_registry().register_named_callback(binding.binding_name, make_ref<dotnet_callback>(type_name, method_name));
      }
    });
  }

  void scripting_system::tick(driver_kernel* kernel, double dt) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scripting_system::tick");
    if (environment_console::is_initialized()) {
      PROFILE_SECTION("scripting_system::tick--environment_console");
      environment_console::poll();
    }
  }

  void scripting_system::shutdown(driver_kernel* kernel) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scripting_system::shutdown");

    do_script_interface_unbinding();
    for (auto& module : loaded_dotnet_modules) {
      unload_dotnet_module(module);
    }
    loaded_dotnet_modules.clear();

    driver_main_lua_script = nullptr;
  }

  ref<assembly> scripting_system::load_dotnet_module(const std::string_view module_path) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scripting_system::load_dotnet_module");
    CORE_LOG_DEBUG("Loading script module from path: {}", module_path);
    filepath path(module_path);
    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Script module path does not exist: {}", module_path);
      return nullptr;
    }
    ref<assembly> asm_ref = subsystem<scripting_environment>::get()->load_dotnet_module(path.string());
    OTHER_ASSERT(asm_ref != nullptr, "Failed to load .NET assembly from path: {}", module_path);

    ostd::vector<callback_binding> bindings = asm_ref->get_native_function_bindings();
    CORE_LOG_DEBUG("Found {} native callback bindings in assembly [{}:{}]", bindings.size(), asm_ref->get_handle(), asm_ref->get_name());
    for (const auto& binding : bindings) {
      auto last_dot = binding.full_type_and_method_name.find_last_of('.');
      std::string type_name = binding.full_type_and_method_name.substr(0, last_dot);
      std::string method_name = binding.full_type_and_method_name.substr(last_dot + 1);
      CORE_LOG_INFO("Native Callback Binding registered: {} -> {}.{}", binding.binding_name, type_name, method_name);
      get_driver().get_interface_registry().register_named_callback(binding.binding_name, make_ref<dotnet_callback>(type_name, method_name));
    }

    return asm_ref;
  }

  void scripting_system::unload_dotnet_module(ref<assembly> module) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scripting_system::unload_dotnet_module");
    if (module == nullptr) {
      CORE_LOG_ERROR("Cannot unload a null module.");
      return;
    }

    CORE_LOG_DEBUG("Unloading script module with ID: {}", module->get_handle());
    subsystem<scripting_environment>::get()->unload_dotnet_module(module);
  }

  lua_host& scripting_system::get_lua_host() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(!subsystem<scripting_environment>::inert, "Scripting environment subsystem is inert, cannot get Lua host.");
    return subsystem<scripting_environment>::get()->get_lua_host();
  }

  lua_script& scripting_system::get_envrc_script() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(envrc != nullptr, "Driver environment runtime script is not loaded.");
    return *envrc;
  }

}  // namespace other