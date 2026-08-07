/**
 * \file other.cpp
 **/
#include "other.hpp"

#include <iostream>

#include <Windows.h>

#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/subsystem.hpp"
#include "core/version.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"
#include "memory/arena.hpp"
#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"
#include "thread/thread_safety.hpp"

#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "driver/subsystem_registry.hpp"

extern other::exit_code other_main(const other::command_line& cmd, const other::config_table& config, const other::subsystem_registry& registry);

namespace other {

  exit_code invoke_other_main(const command_line& cmd, const config_table& config, const std::string_view profile) {
    subsystem_registry registry = register_all_subsystems();
    registry.initialize_profile(profile, &config);

    if (config.diagnostics.verbose) {
      CORE_LOG_INFO("Initialized subsystems for profile '{}':", profile);
      for (const natural_t id : registry.get_initialization_order()) {
        auto it = registry.get_registry().find(id);
        if (it != registry.get_registry().end()) {
          CORE_LOG_INFO("- {}", it->second.name);
        }
      }

      CORE_LOG_INFO("Other Environment version {}", OTHER_ENVIRONMENT_VERSION_STRING);
      CORE_LOG_DEBUG("Environment Config File: {}", cmd.config_file);
      CORE_LOG_DEBUG("Working Directory: {}", std::filesystem::current_path().string());
    }

    exit_code res = FAILURE;
    try {
      PROFILE_SECTION("other::main");
      if (config.diagnostics.verbose) {
        CORE_LOG_DEBUG("Calling Other Main");
      }

      register_main_thread();
      res = other_main(cmd, config, registry);
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Unhandled exception in other_main: {}", e.what());
    } catch (...) {
      CORE_LOG_ERROR("An unknown error occurred in other_main.");
    }

    /// skip logger so that the the exit code can be the last thing in the logs
    // it works cause logger is the only subsystem with no dependencies
    registry.shutdown_all(/* skip logger */ true);
    return res;
  }

  /// hardware faults die stackless — print one like asserts do
  ///  (stderr direct: faults can land after logger shutdown)
  static LONG WINAPI report_unhandled_seh(EXCEPTION_POINTERS* info) {
    const uint32_t code = info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionCode : 0;
    std::cerr << "Unhandled SEH exception 0x" << std::hex << code << std::dec << "\nstacktrace =\n"
              << OTHER_STACKTRACE << std::endl;
    return EXCEPTION_CONTINUE_SEARCH;
  }

  exit_code other_environment_program_entry_point(const command_line& cmd, const config_table& cfg, const std::string_view prof) {
    SetUnhandledExceptionFilter(&report_unhandled_seh);
    exit_code res = invoke_other_main(cmd, cfg, prof);
    CORE_LOG_INFO("Other Environment exit with code: {}", res);
    return res;
  }

  int entry() {
    command_line empty_cmd;
    config_table default_config;
    std::string default_profile = get_default_profile();
    return other_environment_program_entry_point(empty_cmd, default_config, default_profile);
  }

  int entry(int argc, char* argv[]) {
    PROFILE_SECTION("other::entry");

    auto [config_loaded, config, cmd] = read_command_line_and_config(argc, argv);
    if (!config_loaded) {
      if (cmd.diagnostics.help || cmd.diagnostics.usage) {
        return SUCCESS;
      } else {
        return FAILURE;
      }
    }

    std::string profile = get_subsystem_profile(&config);
    return other_environment_program_entry_point(cmd, config, profile);
  }

  namespace detail {

    void finalize_configuration_table(config_table& config, const command_line& cmd);

  }  // namespace detail

  load_config_result read_command_line_and_config(int argc, char* argv[]) {
    command_line cmd = command_line::parse(&argc, argv);
    if (!cmd.valid) {
      return (cmd.diagnostics.help || cmd.diagnostics.usage) ?
        std::make_tuple(true, config_table{}, cmd) :
        std::make_tuple(false, config_table{}, cmd);
    }

    if (cmd.working_directory.has_value()) {
      filepath cwd = cmd.working_directory.value();
      if (!std::filesystem::exists(cwd) || !std::filesystem::is_directory(cwd)) {
        std::println(std::cerr, "[ERROR]: Invalid working directory specified: '{}'", cwd.string());
        return std::make_tuple(false, config_table{}, cmd);
      }

      std::error_code ec;
      std::filesystem::current_path(cmd.working_directory.value(), ec);
      if (ec) {
        std::println(std::cerr, "[ERROR]: Failed to set working directory to '{}': {}", cwd.string(), ec.message());
        return std::make_tuple(false, config_table{}, cmd);
      }
    }

    config_table config = {};
    if (std::filesystem::exists(cmd.config_file)) {
      PROFILE_SECTION("other::entry--load-config");

      config = config_table::load(cmd.config_file);
      if (!config.valid) {
        std::println(std::cerr, "[ERROR]: Failed to load configuration file: '{}'", cmd.config_file);
        return std::make_tuple(false, config, cmd);
      }
      std::println(std::cout, "Loaded configuration from file: '{}'", cmd.config_file);
    } else if (!cmd.config_file.empty()) {
      std::println(std::cout, "[WARNING]: Configuration file '{}' does not exist. Using default configuration.", cmd.config_file);
    } else {
      std::println(std::cout, "No configuration file specified. Using default configuration.");
    }

    detail::finalize_configuration_table(config, cmd);
    return std::make_tuple(true, config, cmd);
  }

  namespace detail {

    void finalize_configuration_table(config_table& config, const command_line& cmd) {
      config.diagnostics.verbose = cmd.diagnostics.verbose;
      if (cmd.diagnostics.verbose) {
        config.diagnostics.verbose = true;
        std::println(std::cout, "Loading Environment with configuration :\n{}\n", config.dump_table_string());
      }

      /// rendering.backend == "headless" is the same as rendering.force-no-window == true, we just check both for ease of use
      bool rendering_enabled = true;
      if ((config.rendering_backend.has_value() && config.rendering_backend.value() == "headless") || config.force_no_window) {
        rendering_enabled = false;
        config.rendering_backend = "headless";
      }
      /// default to opengl
      else if (!config.rendering_backend.has_value()) {
        config.rendering_backend = "opengl";
      }

      if (config.diagnostics.verbose) {
        std::println(std::cout, "Rendering backend: {}", rendering_enabled ? config.rendering_backend.value() : "headless (disabled)");
      }
    }

  }  // namespace detail

  void shutdown_subsystems() {
    PROFILE_SECTION("other::shutdown_subsystems");
    if (!subsystem<scripting_environment>::inert) {
      subsystem<scripting_environment>::get()->shutdown();
    }
    if (!subsystem<renderer_backend>::inert) {
      subsystem<renderer_backend>::get()->shutdown();
    }
    if (!subsystem<physics_environment>::inert) {
      subsystem<physics_environment>::get()->shutdown();
    }
    if (!subsystem<type_database>::inert) {
      subsystem<type_database>::get()->shutdown();
    }
    if (!subsystem<input_system>::inert) {
      subsystem<input_system>::get()->shutdown();
    }
    if (!subsystem<file_system>::inert) {
      subsystem<file_system>::get()->shutdown();
    }
    if (!subsystem<arena>::inert) {
      subsystem<arena>::get()->shutdown();
    }
    if (!subsystem<logger>::inert) {
      subsystem<logger>::get()->shutdown();
    }
  }

}  // namespace other