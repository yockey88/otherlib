/**
 * \file other.cpp
 **/
#include "other.hpp"

#include <iostream>

#include "core/arena.hpp"
#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/version.hpp"
#include "serialization/reflection.hpp"

#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "scripting/dotnet_bindings.hpp"

#include "spdlog/common.h"

#ifndef OTHER_TEST_ENVIRONMENT
/// if not test environment and this is not an other application then we define the extern main function for the static driver
/// \todo: check if the other application is a dynamic driver and define other_main as the dynamic driver entry point (prototype sample in driver/development_driver_loader.cpp)
  #ifndef OTHER_APPLICATION
extern exit_code other_main(const command_line& cmd, const config_table& config);
  #endif
#endif

#if defined(OTHER_DEBUG_BUILD) || defined(OTHER_DEBUG_AS_BUILD)
  #define CATCH_RUNTIME_ERROR(e) OTHER_ASSERT(false, "Runtime error: {}", e.what())
  #define CATCH_EXCEPTION(e) OTHER_ASSERT(false, "Exception: {}", e.what())
  #define CATCH_UNKNOWN_EXCEPTION() OTHER_ASSERT(false, "Unknown exception occurred.")
#else
  #define CATCH_RUNTIME_ERROR(e) CORE_LOG_ERROR("Runtime error: {}", e.what())
  #define CATCH_EXCEPTION(e) CORE_LOG_ERROR("Exception: {}", e.what())
  #define CATCH_UNKNOWN_EXCEPTION() CORE_LOG_ERROR("Unknown exception occurred.")
#endif

namespace other {

  int entry(int argc, char* argv[]) {
    PROFILE_SECTION("other::entry");
    initialize_primary_arena();

    command_line cmd = command_line::parse(&argc, argv);
    if (!cmd.valid) {
      return (cmd.diagnostics.help || cmd.diagnostics.usage) ? SUCCESS : FAILURE;
    }

    if (cmd.working_directory.has_value()) {
      filepath cwd = cmd.working_directory.value();
      if (!std::filesystem::exists(cwd) || !std::filesystem::is_directory(cwd)) {
        std::println(std::cerr, "Invalid working directory specified: '{}'", cwd.string());
        return FAILURE;
      }

      std::error_code ec;
      std::filesystem::current_path(cmd.working_directory.value(), ec);
      if (ec) {
        std::println(std::cerr, "Failed to set working directory to '{}': {}", cwd.string(), ec.message());
        return FAILURE;
      }
    }

    config_table config = {};
    if (std::filesystem::exists(cmd.config_file)) {
      config = config_table::load(cmd.config_file);
      if (!config.valid) {
        std::println(std::cerr, "Failed to load configuration file: '{}'", cmd.config_file);
        return -1;
      }

    } else if (!cmd.config_file.empty()) {
      CORE_LOG_WARN("Configuration file '{}' does not exist. Using default configuration.", cmd.config_file);
    }

    register_log_sinks(config);
    CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
    CORE_LOG_DEBUG("Environment Config File: {}", cmd.config_file);
    CORE_LOG_DEBUG("Working Directory: {}", std::filesystem::current_path().string());

    config.diagnostics.verbose = cmd.diagnostics.verbose;
    const bool rendering_enabled = config.rendering_backend.has_value() && !config.rendering_backend->empty();
    if (rendering_enabled) {
      const bool force_no_window = config.force_no_window;
      subsystem<renderer_backend>::get()->load_backend(config.rendering_backend.value(), config.window_size);
    }

    bind_primary_scripting_environment(config);
    bind_environment_scripts();

    exit_code res = SUCCESS;
    {
      CORE_LOG_INFO("Running Other Environment driver...");
      PROFILE_SECTION("other::main");
      try {
        res = other_main(cmd, config);
      } catch (const std::runtime_error& e) {
        CATCH_RUNTIME_ERROR(e);
        res = FAILURE;
      } catch (const std::exception& e) {
        CATCH_EXCEPTION(e);
        res = FAILURE;
      } catch (...) {
        CATCH_UNKNOWN_EXCEPTION();
        res = FAILURE;
      }
      if (rendering_enabled) {
        subsystem<renderer_backend>::get()->unload_backend();
      }
    }

    cleanup_scripting_environment();

    /// handle exit code
    CORE_LOG_INFO("Other Environment driver finished with exit code: {}", res);

#ifdef OTHER_APPLICATION
    event_callbacks.clear();
#endif

    shutdown_subsystems();
    return res;
  }

  void initialize_primary_arena() {
    arena* primary_arena = subsystem<arena>::get();
    if (primary_arena == nullptr) {
      CORE_LOG_ERROR("Primary arena is null.");
      throw std::runtime_error("Primary arena is null.");
    }
  }

  void bind_primary_scripting_environment(const config_table& config) {
    auto* env = subsystem<scripting_environment>::get();
    env->initialize_script_environment(config);

    filepath other_cs_path = config.get_value<std::string>("scripting.other_cs_path", "C:/OtherEnvironment/dotnet-assemblies/OtherCs.dll");
    env->dotnet_binding_assembly = env->load_dotnet_module(other_cs_path.string());
  }

  native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag) {
    std::string app_name = app_name_str;
    filepath app_data_folder = get_app_data_folder(app_name, create_flag != 0);
    return native_string::new_str(app_data_folder.string());
  }

  void bind_environment_scripts() {
    auto* env = subsystem<scripting_environment>::get();
    /// dotnet binding
    /// we've already loaded OtherCs, so now we bind core functionality, start with the platform directory
    /// functions
    dotnet_host& dn_host = env->get_dotnet_host();
    bind_otherlib_dotnet_functions(dn_host);
  }

  void cleanup_scripting_environment() {
    auto* env = subsystem<scripting_environment>::get();
    env->unload_dotnet_module(env->dotnet_binding_assembly);
    env->dotnet_binding_assembly = nullptr;
    env->shutdown_script_environment();
  }

  spdlog::sink_ptr stdout_sink_fn(const config_table& config) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    return std::make_shared<spdlog::sinks::stdout_sink_mt>();
#endif
  }

  void register_log_sinks(const config_table& config) {
    logger* log = subsystem<logger>::get();
    if (log == nullptr) {
      throw std::runtime_error("Logger subsystem is null.");
    }
    log->set_config(&config);

    log->create_logger("other-core-log", spdlog::level::trace);
    log_sink sink = {
      1,
      "console-sink",
      "%^[%l]%$ %v (%t)",
      (spdlog::level::level_enum)config.core_log_level,
      stdout_sink_fn,
    };
    log_sink file_sink = {
      2,
      "file-sink",
      "[%Y-%m-%d %H:%M:%S -  %t] [%l] %v",
      spdlog::level::trace,
      [](const config_table& config) -> spdlog::sink_ptr {
        return std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.core_log_file, true);
      },
    };
    std::string loggers[] = { "other-core-log" };
    log->register_sink(loggers, sink);
    log->register_sink(loggers, file_sink);
  }

  void shutdown_subsystems() {
    subsystem<scripting_environment>::get()->shutdown();
    subsystem<type_database>::get()->shutdown();
    subsystem<renderer_backend>::get()->shutdown();
    subsystem<arena>::get()->shutdown();
    subsystem<logger>::get()->shutdown();
  }

  namespace {

    void initialize_other_environment_impl(int argc, char* argv[]) {
      config_table config = {};

      other::initialize_primary_arena();
      other::register_log_sinks(config);
      other::bind_primary_scripting_environment(config);
      other::bind_environment_scripts();
    }

  }  // namespace

  void initialize_other_environment() {
    initialize_other_environment_impl(0, nullptr);
  }

  void initialize_other_environment(int argc, char* argv[]) {
    initialize_other_environment_impl(argc, argv);
  }

  void shutdown_other_environment() {
    other::cleanup_scripting_environment();
    other::shutdown_subsystems();
  }

}  // namespace other