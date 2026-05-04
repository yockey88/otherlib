/**
 * \file driver/system/scripting_system.cpp
 **/
#include "driver/systems/scripting_system.hpp"

#include "lua/lua_script.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver.hpp"
#include "scripting/bindings.hpp"
#include "tools/environment_console.hpp"

namespace other {

  void scripting_system::initialize(driver_kernel* kernel) {
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
        if (driver_main_lua_script && driver_main_lua_script->has_symbol("is_command") &&
            driver_main_lua_script->call_function<bool>("is_command", command)) {
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
  }

  void scripting_system::tick(driver_kernel* kernel, double dt) {
    if (environment_console::is_initialized()) {
      environment_console::poll();
    }
  }

  void scripting_system::shutdown(driver_kernel* kernel) {
    do_script_interface_unbinding();
    for (auto& module : loaded_dotnet_modules) {
      unload_dotnet_module(module);
    }
    loaded_dotnet_modules.clear();

    driver_main_lua_script = nullptr;
  }

  ref<assembly> scripting_system::load_dotnet_module(const std::string_view module_path) {
    PROFILE_SECTION("scripting_system::load_dotnet_module");
    CORE_LOG_DEBUG("Loading script module from path: {}", module_path);
    filepath path(module_path);
    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Script module path does not exist: {}", module_path);
      return nullptr;
    }
    return subsystem<scripting_environment>::get()->load_dotnet_module(path.string());
  }

  void scripting_system::unload_dotnet_module(ref<assembly> module) {
    if (module == nullptr) {
      CORE_LOG_ERROR("Cannot unload a null module.");
      return;
    }

    CORE_LOG_DEBUG("Unloading script module with ID: {}", module->get_handle());
    subsystem<scripting_environment>::get()->unload_dotnet_module(module);
  }

  lua_host& scripting_system::get_lua_host() {
    OTHER_ASSERT(!subsystem<scripting_environment>::inert, "Scripting environment subsystem is inert, cannot get Lua host.");
    return subsystem<scripting_environment>::get()->get_lua_host();
  }

  lua_script& scripting_system::get_envrc_script() {
    OTHER_ASSERT(envrc != nullptr, "Driver environment runtime script is not loaded.");
    return *envrc;
  }

}  // namespace other