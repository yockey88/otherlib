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

  void driver::initialize(const command_line& cmd) {
    PROFILE_SECTION("driver::initialize");

    /// set signal catchers
    net_context = std::make_unique<network_context>();
    net_context->signals.async_wait([this](std::error_code ec, int signum) {
      if (!ec) {
        catch_signal(signum);
      } else {
        CORE_LOG_ERROR("Error while waiting for signal: {}", ec.message());
      }
    });

    std::vector<std::string> dotnet_modules = get_config_value<std::vector<std::string>>("scripting", "dotnet-modules");
    for (const auto& module : dotnet_modules) {
      CORE_LOG_DEBUG(" - .NET module to load: {}", module);
      auto assembly = load_dotnet_module(module);
      if (assembly == nullptr) {
        CORE_LOG_ERROR("Failed to load .NET module: {}", module);
      }

      loaded_dotnet_modules.push_back(assembly);
    }

    on_initialize(cmd);
  }

  void driver::shutdown() {
    PROFILE_SECTION("driver::shutdown");
    on_shutdown();

    for (auto& module : loaded_dotnet_modules) {
      unload_dotnet_module(module);
    }
    loaded_dotnet_modules.clear();

    net_context->signals.cancel();
    if (!net_context->io_context.stopped()) {
      net_context->io_context.stop();
    }
    net_context = nullptr;
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

    poll_coroutines();
  }

  scope<renderer> driver::get_renderer() const {
    if (auto* rendering = subsystem<renderer_backend>::get(); !rendering->has_backend()) {
      rendering->load_backend("opengl", { 1280, 720 });
    }
    return make_scope<renderer>();
  }

  ref<assembly> driver::load_dotnet_module(const std::string_view module_path) {
    CORE_LOG_DEBUG("Loading script module from path: {}", module_path);
    filepath path(module_path);
    if (!std::filesystem::exists(path)) {
      CORE_LOG_ERROR("Script module path does not exist: {}", module_path);
      return nullptr;
    }
    return subsystem<scripting_environment>::get()->load_dotnet_module(path.string());
  }

  void driver::unload_dotnet_module(ref<assembly> module) {
    if (module == nullptr) {
      CORE_LOG_ERROR("Cannot unload a null module.");
      return;
    }

    CORE_LOG_DEBUG("Unloading script module with ID: {}", module->get_handle());
    subsystem<scripting_environment>::get()->unload_dotnet_module(module);
  }

  void driver::launch_detached_process(const filepath& working_dir, const filepath& exe_name, const std::vector<std::string>& args) {
    if (!std::filesystem::exists(working_dir) || !std::filesystem::is_directory(working_dir)) {
      CORE_LOG_ERROR("Working directory does not exist or is not a directory: {}", working_dir.string());
      return;
    }
#ifdef OTHER_ENVIRONMENT_WINDOWS
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    auto working_dir_str = working_dir.string();
    std::wstring wworking_dir = std::wstring(working_dir_str.begin(), working_dir_str.end());

    auto exe_name_str = exe_name.string();
    std::wstring wexe_name = std::wstring(exe_name_str.begin(), exe_name_str.end());

    std::vector<std::wstring> wargs;
    for (const auto& arg : args) {
      wargs.push_back(std::wstring(arg.begin(), arg.end()));
    }

    std::wstring full_command = wexe_name;
    for (const auto& warg : wargs) {
      full_command += L" " + warg;
    }
    std::string str_full_command(full_command.begin(), full_command.end());

    // Start the child process.
    if (!CreateProcessW(
          // no name, use command line
          nullptr, full_command.data(),
          // make nothing inheritable
          nullptr, nullptr, FALSE,
          /// completely new, NEW_CONSOLE is only for dev
          CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
          /// Use parent's environment block
          nullptr,
          /// Set working directory
          wworking_dir.data(),
          // Pointer to STARTUPINFOW structure and PROCESS_INFORMATION structure
          &si, &pi
        )) {
      CORE_LOG_ERROR("CreateProcess failed (error {}): \n   [command: {}]\n", GetLastError(), str_full_command);
      return;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
  #error "UNIMPLEMENTED PLATFORM"
#endif
  }

  void driver::add_live_coroutine(task handle) {
    live_coroutines.push_back({ handle });
  }

  void driver::poll_coroutines() {
    for (auto it = live_coroutines.begin(); it != live_coroutines.end();) {
      it->handle();
      if (it->handle.coro_handle.done()) {
        it->handle.coro_handle.destroy();
        it = live_coroutines.erase(it);
      } else {
        ++it;
      }
    }
  }

}  // namespace other