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
#include "core/version.hpp"
#include "input/input_system.hpp"
#include "serialization/reflection.hpp"

#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "scripting/dotnet_bindings.hpp"
#include "scripting/lua_bindings.hpp"

extern exit_code other_main(const command_line& cmd, const config_table& config);

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
        std::println(std::cerr, "[ERROR]: Invalid working directory specified: '{}'", cwd.string());
        return FAILURE;
      }

      std::error_code ec;
      std::filesystem::current_path(cmd.working_directory.value(), ec);
      if (ec) {
        std::println(std::cerr, "[ERROR]: Failed to set working directory to '{}': {}", cwd.string(), ec.message());
        return FAILURE;
      }
    }

    config_table config = {};
    if (std::filesystem::exists(cmd.config_file)) {
      PROFILE_SECTION("other::entry--load-config");

      config = config_table::load(cmd.config_file);
      if (!config.valid) {
        std::println(std::cerr, "[ERROR]: Failed to load configuration file: '{}'", cmd.config_file);
        return -1;
      }
      std::println(std::cout, "Loaded configuration from file: '{}'", cmd.config_file);
    } else if (!cmd.config_file.empty()) {
      std::println(std::cout, "[WARNING]: Configuration file '{}' does not exist. Using default configuration.", cmd.config_file);
    } else {
      std::println(std::cout, "No configuration file specified. Using default configuration.");
    }

    register_log_sinks(config);
    if (cmd.diagnostics.verbose) {
      config.diagnostics.verbose = true;
      CORE_LOG_DEBUG("Loading Environment with configuration :\n{}\n", config.dump_table_string());
    }

    CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
    CORE_LOG_DEBUG("Environment Config File: {}", cmd.config_file);
    CORE_LOG_DEBUG("Working Directory: {}", std::filesystem::current_path().string());

    config.diagnostics.verbose = cmd.diagnostics.verbose;

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

    /// if rendering is enabled and we are not forcing headless mode, load the rendering backend
    if (rendering_enabled) {
      PROFILE_SECTION("other::entry--initialize-renderer-backend");
      subsystem<renderer_backend>::get()->load_backend(config, config.rendering_backend.value(), config.window_size);
    }

    bool force_disable_physics = config.get_value<bool>("physics.force-disable-physics", false);
    if (!force_disable_physics) {
      bind_physics_environment(config);
    }

    bool force_disable_scripting = config.get_value<bool>("scripting.force-disable-scripting", false);
    if (!force_disable_scripting) {
      PROFILE_SECTION("other::entry--initialize-scripting");
      bind_primary_scripting_environment(config);
      bind_environment_scripts();
    }

    subsystem<input_system>::get()->initialize();
    /// \note maybe not doing this here anymore
    /// \todo handle other-driver registration here, this includes loading everything not pulled from environment config file
    ///        and registering/initializing all user-facing APIs (this includes things like registering user-facing log, registering user events, etc)

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

      subsystem<input_system>::get()->shutdown();
      if (rendering_enabled) {
        subsystem<renderer_backend>::get()->unload_backend();
      }
    }

    if (!force_disable_scripting) {
      PROFILE_SECTION("other::entry--cleanup-scripting");
      cleanup_scripting_environment();
    }

    if (!force_disable_physics) {
      PROFILE_SECTION("other::entry--shutdown-physics-environment");
      cleanup_physics_environment();
    }

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

  void bind_physics_environment(const config_table& config) {
    PROFILE_SECTION("other::bind-physics-environment");
    auto* phys_env = subsystem<physics_environment>::get();
    OTHER_ASSERT(phys_env != nullptr, "Physics environment subsystem is null.");

    phys_env->load_backend(config);
    phys_env->initialize_physics_environment(config);
  }

  void bind_primary_scripting_environment(const config_table& config) {
    PROFILE_SECTION("other::bind-primary-scripting-environment");
    auto* env = subsystem<scripting_environment>::get();
    env->initialize_script_environment(config);

    filepath other_cs_path = config.get_value<std::string>(configuration::kOtherCSharp, "C:/OtherEnvironment/dotnet-assemblies/OtherCs.dll");
    OTHER_ASSERT(std::filesystem::exists(other_cs_path), "OtherCs.dll not found at path: {}", other_cs_path.string());
    CORE_LOG_DEBUG("Using OtherCs.dll at path: {}", other_cs_path.string());
    env->dotnet_binding_assembly = env->load_dotnet_module(other_cs_path.string());
  }

  native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag) {
    std::string app_name = app_name_str;
    filepath app_data_folder = get_app_data_folder(app_name, create_flag != 0);
    return native_string::new_str(app_data_folder.string());
  }

  void bind_environment_scripts() {
    PROFILE_SECTION("other::bind-environment-scripts");
    auto* env = subsystem<scripting_environment>::get();
    /// dotnet binding
    /// we've already loaded OtherCs, so now we bind core functionality, start with the platform directory
    /// functions
    dotnet_host& dn_host = env->get_dotnet_host();
    lua_host& l_host = env->get_lua_host();
    bind_otherlib_dotnet_functions(dn_host);
    bind_otherlib_lua_functions(l_host);
  }

  void cleanup_scripting_environment() {
    PROFILE_SECTION("other::cleanup-scripting-environment");
    auto* env = subsystem<scripting_environment>::get();
    env->unload_dotnet_module(env->dotnet_binding_assembly);
    env->dotnet_binding_assembly = nullptr;
    env->shutdown_script_environment();
  }

  void cleanup_physics_environment() {
    PROFILE_SECTION("other::cleanup-physics-environment");
    auto* phys_env = subsystem<physics_environment>::get();
    phys_env->shutdown_physics_environment();
    phys_env->unload_backend();
  }

  spdlog::sink_ptr stdout_sink_fn(const config_table& config) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif
  }

  void register_log_sinks(const config_table& config) {
    PROFILE_SECTION("other::register-log-sinks");

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
    PROFILE_SECTION("other::shutdown_subsystems");
    subsystem<scripting_environment>::get()->shutdown();
    subsystem<type_database>::get()->shutdown();
    subsystem<physics_environment>::get()->shutdown();
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