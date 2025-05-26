/**
 * \file other.cpp
 **/
#include "other.hpp"

#include <print>

#include "core/arena.hpp"
#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "renderer/renderer_backend.hpp"

#include "spdlog/common.h"


namespace other {

  namespace environment {

    /// arena initialization functions
    void initialize_primary_arena();

    /// logger initialization functions
    spdlog::sink_ptr stdout_sink_fn();
    void register_log_sinks(const config_table& config);

    void shutdown_subsystems();

    int entry(int argc, char* argv[]) {
      command_line cmd = command_line::parse(&argc, argv);
      if (!cmd.valid) {
        return (cmd.diagnostics.help || cmd.diagnostics.usage) ? 0 : -1;
      }
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Loading configuration file: {}", cmd.config_file);
      }
      config_table config = config_table::load(cmd.config_file);

      if (!config.valid) {
        CORE_LOG_ERROR("Failed to load configuration file: {}", cmd.config_file);
        return -1;
      }
      config.diagnostics.verbose = cmd.diagnostics.verbose;

      {
        struct subsystem_shutdown_helper {
          ~subsystem_shutdown_helper() {
            shutdown_subsystems();
          }
        } shutdown_helper;

        register_log_sinks(config);
        CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);

        initialize_primary_arena();

        /// automatic shutdown in destructor of subsystem_shutdown_helper

        CORE_LOG_INFO("Configuration loaded successfully from: {}", cmd.config_file);
        auto [driver_instance, driver_name] = driver::create(config);

        if (driver_instance != nullptr) {
          CORE_LOG_INFO("Running Other Environment driver '{}'", driver_name);
          driver_instance->initialize();
          driver_instance->run();
          driver_instance->shutdown();
        }

        driver::destroy(driver_name, driver_instance);
        CORE_LOG_INFO("Other Environment driver '{}' has finished unloading.", driver_name);
      }
      return 0;
    }

    void initialize_primary_arena() {
      arena* primary_arena = subsystem<arena>::get();
      if (primary_arena == nullptr) {
        CORE_LOG_ERROR("Primary arena is null.");
        throw std::runtime_error("Primary arena is null.");
      }
    }

    spdlog::sink_ptr stdout_sink_fn() {
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

      log->create_logger("other-core-log", spdlog::level::trace);
      other::log_sink sink = {
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
      subsystem<renderer_backend>::get()->shutdown();
      subsystem<arena>::get()->shutdown();
      subsystem<logger>::get()->shutdown();
    }

  }  // namespace environment

}  // namespace other