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

#include "plugin/library_handle.hpp"

#include "driver/terminal_driver.hpp"

namespace other {

  int run_otherdriver(int argc, char* argv[]);

  namespace environment {

    /// arena initialization functions
    void initialize_primary_arena();

    /// logger initialization functions
    spdlog::sink_ptr stdout_sink_fn();
    void register_log_sinks();

    void shutdown_subsystems();

    int entry(int argc, char* argv[]) {
      struct subsystem_shutdown_helper {
        ~subsystem_shutdown_helper() {
          shutdown_subsystems();
        }
      } shutdown_helper;

      register_log_sinks();
      initialize_primary_arena();

      /// automatic shutdown in destructor of subsystem_shutdown_helper
      return run_otherdriver(argc, argv);
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

    void register_log_sinks() {
      logger* log = subsystem<logger>::get();
      if (log == nullptr) {
        throw std::runtime_error("Logger subsystem is null.");
      }

      log->create_logger("other-core-log", spdlog::level::trace);
      other::log_sink sink = {
        1,
        "console-sink",
        "%^[%l]%$ %v",
        spdlog::level::trace,
        stdout_sink_fn,
      };
      std::string loggers[] = { "other-core-log" };
      log->register_sink(loggers, sink);
    }

    void shutdown_subsystems() {
      subsystem<arena>::get()->shutdown();
      subsystem<logger>::get()->shutdown();
    }

  }  // namespace environment

  int run_otherdriver(int argc, char* argv[]) {
    command_line cmd = command_line::parse(&argc, argv);
    if (!cmd.valid) {
      return (cmd.diagnostics.help || cmd.diagnostics.usage) ? 0 : -1;
    }

    CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);

    if (cmd.diagnostics.verbose) {
      CORE_LOG_DEBUG("Loading configuration file: {}", cmd.config_file);
    }
    config_table config = config_table::load(cmd.config_file);
    if (!config.valid) {
      CORE_LOG_ERROR("Failed to load configuration file: {}", cmd.config_file);
      return -1;
    }

    driver* driver_instance = nullptr;

    std::string driver_path = config.dynamic_driver_rel_path.value_or("");
    std::string driver_name = "";

    /// if no path then run built-in driver/event loop with environment terminal
    if (driver_path.empty()) {
      if (cmd.diagnostics.verbose) {
        CORE_LOG_INFO("No dynamic driver path provided, running built-in environment terminal.");
      }
      driver_instance = create_terminal_driver(config);
    }
    /// otherwise attempt to load the driver and run it
    else {
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Attempting to load driver {}", driver_path);
      }

      library_handle* lib_handle = plugin::load_plugin_library(driver_path);
      if (lib_handle == nullptr) {
        CORE_LOG_ERROR("Failed to load plugin library: {}", driver_path);
        return -1;
      }
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Loaded plugin library: {}", driver_path);
      }
      driver_name = filepath(driver_path).filename().stem().string();
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Driver name: {}", driver_name);
      }

      symbol& sym = lib_handle->get_symbol("create_driver");
      if (sym.address == nullptr) {
        CORE_LOG_ERROR("Failed to load symbol 'create_driver' from plugin '{}'", driver_path);
        return -1;
      }
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Loaded symbol 'create_driver' from plugin '{}'", driver_path);
      }

      driver* (*fn)(const config_table*) = sym.get_function<driver* (*)(const config_table*)>();
      driver_instance = fn(&config);
    }

    if (driver_instance == nullptr) {
      CORE_LOG_ERROR("Failed to create driver instance.");
    } else {
      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Driver instance created, running....");
      }
      /// \todo add stack system to push new drivers and pop and shutdown when the stack is empty

      driver_instance->initialize();
      driver_instance->run();
      driver_instance->shutdown();

      if (cmd.diagnostics.verbose) {
        CORE_LOG_DEBUG("Driver instance shutdown.");
      }
    }

    if (driver_path.empty()) {
      destroy_terminal_driver(driver_instance);
    } else {
      library_handle* lib_handle = plugin::get_plugin_library(driver_name);
      if (lib_handle == nullptr) {
        CORE_LOG_ERROR("Failed to get plugin library: {}", driver_name);
        return -1;
      }

      symbol& sym = lib_handle->get_symbol("destroy_driver");
      if (sym.address == nullptr) {
        CORE_LOG_ERROR("Failed to load symbol 'destroy_driver' from plugin '{}'", driver_path);
        return -1;
      }
      sym.get_function<void (*)(driver*)>()(driver_instance);

      plugin::unload_plugin_library(driver_path);
    }

    plugin::unload_all_plugin_libraries();

    return 0;
  }

}  // namespace other