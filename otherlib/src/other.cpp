/**
 * \file other.cpp
 **/
#include "other.hpp"

#include <iostream>

#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/arena.hpp"
#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "core/subsystem.hpp"
#include "core/version.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"
#include "serialization/reflection.hpp"

#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "driver/subsystem_registry.hpp"

extern other::exit_code other_main(const other::command_line& cmd, const other::config_table& config);

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

    auto [config_loaded, config, cmd] = read_command_line_and_config(argc, argv);
    if (!config_loaded) {
      if (cmd.diagnostics.help || cmd.diagnostics.usage) {
        return SUCCESS;
      } else {
        return FAILURE;
      }
    }

    subsystem_registry registry = register_all_subsystems();
    const std::string_view profile = get_subsystem_profile(&config);
    registry.initialize_profile(profile, &config);

    if (config.diagnostics.verbose) {
      CORE_LOG_INFO("Initialized subsystems for profile '{}':", profile);
      for (const natural_t id : registry.get_initialization_order()) {
        auto it = registry.get_registry().find(id);
        if (it != registry.get_registry().end()) {
          CORE_LOG_INFO("- {}", it->second.name);
        }
      }

      CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
      CORE_LOG_DEBUG("Environment Config File: {}", cmd.config_file);
      CORE_LOG_DEBUG("Working Directory: {}", std::filesystem::current_path().string());
    }

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
    }

    /// simply want the exit code to be the last thing in the logs
    registry.shutdown_all(/* skip logger */ true);
    CORE_LOG_INFO("Other Environment exit with code: {}", res);
    shutdown_subsystems();
    return res;
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
    subsystem<scripting_environment>::get()->shutdown();
    subsystem<type_database>::get()->shutdown();
    subsystem<physics_environment>::get()->shutdown();
    subsystem<renderer_backend>::get()->shutdown();
    subsystem<input_system>::get()->shutdown();
    subsystem<file_system>::get()->shutdown();
    subsystem<arena>::get()->shutdown();
    subsystem<logger>::get()->shutdown();
  }

}  // namespace other