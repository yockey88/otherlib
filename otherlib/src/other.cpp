/**
 * \file other.cpp
 **/
#include "other.hpp"

#include <iostream>
#include <print>

#include "core/arena.hpp"
#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/version.hpp"
#include "serialization/reflection.hpp"

#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

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
    config_table config = config_table::load(cmd.config_file);
    if (!config.valid) {
      std::cerr << "Failed to load configuration file: " << cmd.config_file << std::endl;
      return -1;
    }

    register_log_sinks(config);
    CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);

    config.diagnostics.verbose = cmd.diagnostics.verbose;
    const bool rendering_enabled = config.rendering_backend.has_value() && !config.rendering_backend->empty();
    if (rendering_enabled) {
      subsystem<renderer_backend>::get()->load_backend(config.rendering_backend.value(), config.window_size);
    }

    bind_primary_scripting_environment();
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

  void bind_primary_scripting_environment() {
    subsystem<scripting_environment>::get()->initialize_script_environment();
  }

  void bind_environment_scripts() {
  }

  void cleanup_scripting_environment() {
    subsystem<scripting_environment>::get()->shutdown_script_environment();
  }

  spdlog::sink_ptr stdout_sink_fn() {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    return std::make_shared<spdlog::sinks::stdout_sink_mt>();
#endif
  }

  spdlog::sink_ptr stdout_sink_fn();
  void register_log_sinks(const config_table& config) {
    logger* log = subsystem<logger>::get();
    if (log == nullptr) {
      throw std::runtime_error("Logger subsystem is null.");
    }

    log->create_logger("other-core-log", spdlog::level::trace);
    log_sink sink = {
      1,
      "console-sink",
      "%^[%l]%$ %v",
      (spdlog::level::level_enum)config.core_log_level,
      stdout_sink_fn,
    };
    std::string loggers[] = { "other-core-log" };
    log->register_sink(loggers, sink);
  }

  void shutdown_subsystems() {
    subsystem<scripting_environment>::get()->shutdown();
    subsystem<type_database>::get()->shutdown();
    subsystem<renderer_backend>::get()->shutdown();
    subsystem<arena>::get()->shutdown();
    subsystem<logger>::get()->shutdown();
  }

}  // namespace other