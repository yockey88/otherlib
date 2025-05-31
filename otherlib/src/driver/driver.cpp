/**
 * \file driver/driver.cpp
 **/
#include "driver/driver.hpp"

#include <SDL3/SDL.h>

#include "core/logger.hpp"
#include "driver/terminal_driver.hpp"
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_events.h"

namespace other {

  void driver::initialize() {
    /// load necessary resources from config
    ///  we can be certain that any thread-dependent resources are loaded on the thread they will be used by client driver
    auto* rendering = subsystem<renderer_backend>::get();
    if (configuration().rendering_backend.has_value()) {
      CORE_LOG_INFO("Loading rendering backend: {}", configuration().rendering_backend.value());
      rendering->load_backend(configuration().rendering_backend.value());
    }

    on_initialize();
  }

  void driver::shutdown() {
    on_shutdown();

    auto* rendering = subsystem<renderer_backend>::get();
    rendering->unload_backend();
  }

  std::pair<driver*, std::string> driver::create(const config_table& config) {
    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      CORE_LOG_INFO("No dynamic driver path provided, running built-in environment terminal.");
      driver_instance = create_terminal_driver(config);
      driver_name = "terminal_driver";
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
    if (name == "terminal_driver") {
      destroy_terminal_driver(instance);
    } else {
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
  }

  void driver::pump_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
          if (event.window.windowID == SDL_GetWindowID(subsystem<renderer_backend>::get()->get_window())) {
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
      rendering->load_backend("opengl");
    }
    return make_scope<renderer>();
  }

}  // namespace other