/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include "core/logger.hpp"

#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  void driver::initialize() {
    PROFILE_SECTION("driver::initialize");
    on_initialize();
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    on_shutdown();
  }

  std::pair<driver*, std::string> driver::create(const config_table& config) {
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_ERROR("No driver path specified, using default driver.");
      return { nullptr, "" };
    }
    /// otherwise attempt to load the driver and run it
    else {
      CORE_LOG_INFO("Attempting to load dynamic driver from path: {}", driver_path);

      library_handle* lib_handle = plugin::load_plugin_library(driver_path);
      if (lib_handle == nullptr) {
        CORE_LOG_ERROR("Failed to load plugin library: {}", driver_path);
        return { nullptr, "" };
      }
      driver_name = filepath(driver_path).filename().stem().string();
      CORE_LOG_DEBUG("Loaded plugin library [{}] : {}", driver_name, driver_path);

      auto sym_res = lib_handle->get_symbol("create_driver");
      if (!sym_res.has_value()) {
        CORE_LOG_ERROR("Failed to get symbol 'create_driver' from plugin '{}'", driver_path);
        return { nullptr, "" };
      }
      symbol& sym = sym_res.value();
      if (sym.address == nullptr) {
        CORE_LOG_ERROR("Failed to load symbol 'create_driver' from plugin '{}'", driver_path);
        return { nullptr, "" };
      }

      CORE_LOG_DEBUG("calling 'create_driver' from plugin [{}]", driver_name);
      driver* (*fn)(const config_table*) = sym.get_function<driver* (*)(const config_table*)>();
      driver_instance = fn(&config);
      CORE_LOG_DEBUG("Loaded driver [{}]", driver_name);
    }

    return { driver_instance, driver_name };
  }

  void driver::destroy(const std::string& name, driver* instance) {
    OTHER_ASSERT(instance != nullptr, "Cannot destroy a null driver instance.");

    library_handle* lib_handle = plugin::get_plugin_library(name);
    if (lib_handle == nullptr) {
      CORE_LOG_ERROR("Failed to get plugin library: {}", name);
      return;
    }

    auto sym_res = lib_handle->get_symbol("destroy_driver");
    if (!sym_res.has_value()) {
      CORE_LOG_ERROR("Failed to get symbol 'destroy_driver' from plugin '{}'", name);
      return;
    }
    symbol& sym = sym_res.value();
    if (sym.address == nullptr) {
      CORE_LOG_ERROR("Failed to load symbol 'destroy_driver' from plugin '{}'", name);
      return;
    }

    CORE_LOG_DEBUG("calling 'destroy_driver' from plugin [{}]", name);
    sym.get_function<void (*)(driver*)>()(instance);
    plugin::unload_plugin_library(name);
  }

  void driver::pump_events() {
    PROFILE_SECTION("driver::pump_events");
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
          if (event.window.windowID == SDL_GetWindowID(subsystem<renderer_backend>::get()->get_main_window())) {
            shutdown_requested = true;
          }
        } break;

        default: {
        } break;
      }

      subsystem<renderer_backend>::get()->handle_event(&event);
      on_event(&event);
    }
  }

  scope<renderer> driver::get_renderer() const {
    if (auto* rendering = subsystem<renderer_backend>::get(); !rendering->has_backend()) {
      rendering->load_backend("opengl", { 1280, 720 });
    }
    return make_scope<renderer>();
  }

  ref<assembly> driver::load_dotnet_module(const std::string_view module_path) {
    CORE_LOG_DEBUG("Loading script module from path: {}", module_path);
    std::filesystem::path path(module_path);
    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Script module path does not exist: {}", module_path);
      return nullptr;
    }
    return subsystem<scripting_environment>::get()->load_dotnet_module(path.string());
  }

  void driver::unload_dotnet_module(ref<assembly> module) {
    CORE_LOG_DEBUG("Unloading script module with ID: {}", module->get_handle());
    subsystem<scripting_environment>::get()->unload_dotnet_module(module);
  }

}  // namespace other