/**
 * \file driver/subsystem_registry.cpp
 **/
#include "driver/subsystem_registry.hpp"

#include "core/logger.hpp"
#include "core/logger_sinks.hpp"
#include "core/subsystem.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"
#include "memory/arena.hpp"

#include "audio/audio_environment.hpp"
#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver_mounts.hpp"
#include "scripting/bindings.hpp"

namespace other {
  namespace detail {

    bool is_valid_profile_name(const std::string_view profile_name) {
      return profile_name == subsystem_profile::kMinimalProfileName ||
        profile_name == subsystem_profile::kMinimalRenderingProfileName ||
        profile_name == subsystem_profile::kMinimalPhysicsProfileName ||
        profile_name == subsystem_profile::kMinimalScriptingProfileName ||
        profile_name == subsystem_profile::kHeadlessProfileName ||
        profile_name == subsystem_profile::kFullProfileName;
    }

    void initialize_logger(const config_table* config);
    void initialize_arena(const config_table* config);
    void initialize_file_system(const config_table* config);
    void initialize_input_system(const config_table* config);
    void initialize_type_database(const config_table* config);
    void initialize_physics_environment(const config_table* config);
    void initialize_renderer_backend(const config_table* config);
    void initialize_scripting_environment(const config_table* config);
    void initialize_audio_environment(const config_table* config);

    void shutdown_logger();
    void shutdown_arena();
    void shutdown_file_system();
    void shutdown_input_system();
    void shutdown_type_database();
    void shutdown_physics_environment();
    void shutdown_renderer_backend();
    void shutdown_scripting_environment();
    void shutdown_audio_environment();

  }  // namespace detail

  void subsystem_registry::register_subsystem(const subsystem_definition& def) {
    if (registry.find(FNV(def.name)) != registry.end()) {
      CORE_LOG_ERROR("Subsystem with name '{}' is already registered.", def.name);
      return;
    }

    registry.emplace(FNV(def.name), def);
  }

  void subsystem_registry::override_subsystem_initialization(const std::string_view name, subsystem_initializer init_fn) {
    auto it = registry.find(FNV(name));
    if (it == registry.end()) {
      CORE_LOG_ERROR("Subsystem with name '{}' is not registered.", name);
      return;
    }
    it->second.initialize_fn = init_fn;
  }

  void subsystem_registry::override_subsystem_shutdown(const std::string_view name, subsystem_shutdown shutdown_fn) {
    auto it = registry.find(FNV(name));
    if (it == registry.end()) {
      CORE_LOG_ERROR("Subsystem with name '{}' is not registered.", name);
      return;
    }
    it->second.shutdown_fn = shutdown_fn;
  }

  void subsystem_registry::initialize_profile(const std::string_view profile, const config_table* config) {
    current_profile = profile;
    std::println(std::cout, "Initializing subsystems for profile '{}'", current_profile);
    resolve_dependency_list_and_do_initialization(get_required_subsystems_for_profile(current_profile), config);
  }

  void subsystem_registry::resolve_dependency_list_and_do_initialization(std::span<const std::string_view> requested_systems, const config_table* config) {
    initialization_order.clear();
    initialization_order = resolve_dependencies(requested_systems, subsystem_definition{});

    for (const natural_t id : initialization_order) {
      auto it = registry.find(id);
      if (it == registry.end()) {
        throw std::runtime_error(std::format("Subsystem with ID {} is not registered.", id));
        continue;
      }
      const subsystem_definition& def = it->second;

      if (def.initialize_fn == nullptr) {
        throw std::runtime_error(std::format("Subsystem '{}' does not have an initialization function.", def.name));
      }
      std::println(std::cout, " - subsystem init: '{}'", def.name);
      def.initialize_fn(config);
    }
  }

  void subsystem_registry::shutdown_all(bool skip_logger) {
    for (auto it = initialization_order.rbegin(); it != initialization_order.rend(); ++it) {
      const natural_t id = *it;
      auto reg_it = registry.find(id);
      if (reg_it == registry.end()) {
        std::println(std::cerr, "Failed to find subsystem with ID {} during shutdown. Skipping.", id);
        continue;
      }
      const subsystem_definition& def = reg_it->second;

      if (skip_logger && def.name == subsystem_profile::kLogger) {
        continue;
      }

      if (def.shutdown_fn == nullptr) {
        std::println(std::cerr, "Subsystem '{}' does not have a shutdown function. Skipping.", def.name);
        continue;
      }
      def.shutdown_fn();
    }
  }

  bool subsystem_registry::profile_includes_scripting(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName ||
      profile_name == subsystem_profile::kMinimalScriptingProfileName ||
      profile_name == subsystem_profile::kHeadlessProfileName;
  }

  bool subsystem_registry::profile_includes_physics(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName ||
      profile_name == subsystem_profile::kMinimalPhysicsProfileName ||
      profile_name == subsystem_profile::kHeadlessProfileName;
  }

  bool subsystem_registry::profile_includes_rendering(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName ||
      profile_name == subsystem_profile::kMinimalRenderingProfileName;
  }

  bool subsystem_registry::profile_includes_vm(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName ||
      profile_name == subsystem_profile::kMinimalScriptingProfileName ||
      profile_name == subsystem_profile::kHeadlessProfileName;
  }

  bool subsystem_registry::profile_includes_scene(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName ||
      profile_name == subsystem_profile::kMinimalScriptingProfileName ||
      profile_name == subsystem_profile::kHeadlessProfileName;
  }

  bool subsystem_registry::profile_includes_audio(const std::string_view profile_name) {
    return profile_name == subsystem_profile::kFullProfileName;
  }

  void subsystem_registry::activate_necessary_subsystems_for_profile(const std::string_view profile, const config_table* config) {
    /// one by one set inert flag to true if not needed in the profile
    subsystem<scripting_environment>::inert = !profile_includes_scripting(profile);
    subsystem<physics_environment>::inert = !profile_includes_physics(profile);
    subsystem<renderer_backend>::inert = !profile_includes_rendering(profile);
    subsystem<scripting_environment>::inert = !profile_includes_vm(profile);
    subsystem<scripting_environment>::inert = !profile_includes_scene(profile);
  }

  std::vector<natural_t> subsystem_registry::resolve_dependencies(std::span<const std::string_view> requested_systems, const subsystem_definition& def) {
    std::vector<natural_t> result;
    std::unordered_map<natural_t, bool> visited;

    std::function<void(const subsystem_definition&)> visit = [&](const subsystem_definition& def) {
      if (visited[FNV(def.name)]) {
        return;
      }
      visited[FNV(def.name)] = true;

      for (const std::string_view& dep : def.depends_on) {
        auto it = registry.find(FNV(dep));
        if (it == registry.end()) {
          std::println(std::cerr, "Subsystem '{}' depends on subsystem '{}' which is not registered. Skipping dependency.", def.name, dep);
          continue;
        }
        visit(it->second);
      }

      result.push_back(FNV(def.name));
    };

    for (const std::string_view& name : requested_systems) {
      natural_t id = FNV(name);
      auto it = registry.find(id);
      if (it == registry.end()) {
        CORE_LOG_ERROR("Requested subsystem '{}' is not registered.", name);
        continue;
      }
      visit(it->second);
    }
    return result;
  }

  subsystem_registry register_all_subsystems() {
    subsystem_registry registry;
    registry.register_subsystem({
      "logger",
      {},
      detail::initialize_logger,
      detail::shutdown_logger,
    });
    registry.register_subsystem({
      "arena",
      subsystem_description<arena>::dependency_names,
      detail::initialize_arena,
      detail::shutdown_arena,
    });
    registry.register_subsystem({
      "file_system",
      subsystem_description<file_system>::dependency_names,
      detail::initialize_file_system,
      detail::shutdown_file_system,
    });
    registry.register_subsystem({
      "input_system",
      subsystem_description<input_system>::dependency_names,
      detail::initialize_input_system,
      detail::shutdown_input_system,
    });
    registry.register_subsystem({
      "type_database",
      subsystem_description<type_database>::dependency_names,
      detail::initialize_type_database,
      detail::shutdown_type_database,
    });
    registry.register_subsystem({
      "physics_environment",
      subsystem_description<physics_environment>::dependency_names,
      detail::initialize_physics_environment,
      detail::shutdown_physics_environment,
    });
    registry.register_subsystem({
      "renderer_backend",
      subsystem_description<renderer_backend>::dependency_names,
      detail::initialize_renderer_backend,
      detail::shutdown_renderer_backend,
    });
    registry.register_subsystem({
      "scripting_environment",
      subsystem_description<scripting_environment>::dependency_names,
      detail::initialize_scripting_environment,
      detail::shutdown_scripting_environment,
    });
    registry.register_subsystem({
      "audio_environment",
      subsystem_description<audio_environment>::dependency_names,
      detail::initialize_audio_environment,
      detail::shutdown_audio_environment,
    });

    return registry;
  }

  std::string get_default_profile() {
    return std::string{ subsystem_profile::kMinimalProfileName };
  }

  std::string get_subsystem_profile(const config_table* config) {
    std::vector<std::string> profile;
    if (config == nullptr) {
      return std::string{ subsystem_profile::kFullProfileName };
    }

    if (auto prof = config->try_get_value<std::string>("environment.profile"); prof.has_value() && !prof->empty()) {
      if (detail::is_valid_profile_name(*prof)) {
        std::println(std::cout, "Using profile '{}' from config.", *prof);
        return *prof;
      } else {
        std::println(std::cerr, "Invalid profile name '{}' in config. Defaulting to full profile.", *prof);
        return std::string{ subsystem_profile::kFullProfileName };
      }
    }

    bool rendering_disabled = (config->rendering_backend.has_value() && config->rendering_backend.value() == "headless") || config->force_no_window;
    bool force_disable_physics = config->get_value<bool>("physics.force-disable-physics", false);
    bool force_disable_scripting = config->get_value<bool>("scripting.force-disable-scripting", false);

    /// all disabled = minimal
    if (rendering_disabled && force_disable_physics && force_disable_scripting) {
      return std::string{ subsystem_profile::kMinimalProfileName };
    }

    // all not disabled (enabled) = full
    if (!rendering_disabled && !force_disable_physics && !force_disable_scripting) {
      return std::string{ subsystem_profile::kFullProfileName };
    }

    if (!rendering_disabled && force_disable_physics && force_disable_scripting) {
      return std::string{ subsystem_profile::kMinimalRenderingProfileName };
    }
    if (rendering_disabled && !force_disable_physics && force_disable_scripting) {
      return std::string{ subsystem_profile::kMinimalPhysicsProfileName };
    }
    if (rendering_disabled && force_disable_physics && !force_disable_scripting) {
      return std::string{ subsystem_profile::kMinimalScriptingProfileName };
    }

    return std::string{ subsystem_profile::kMinimalProfileName };
  }

  std::span<const std::string_view> get_required_subsystems_for_profile(const std::string_view profile_name) {
    if (profile_name == subsystem_profile::kMinimalProfileName) {
      return subsystem_profile::kMinimalProfile;
    }
    if (profile_name == subsystem_profile::kMinimalRenderingProfileName) {
      return subsystem_profile::kMinimalRenderingProfile;
    }
    if (profile_name == subsystem_profile::kMinimalPhysicsProfileName) {
      return subsystem_profile::kMinimalPhysicsProfile;
    }
    if (profile_name == subsystem_profile::kMinimalScriptingProfileName) {
      return subsystem_profile::kMinimalScriptingProfile;
    }
    if (profile_name == subsystem_profile::kHeadlessProfileName) {
      return subsystem_profile::kHeadlessProfile;
    }
    return subsystem_profile::kFullProfile;
  }

  namespace detail {

    /**
     * @note exceptions are fine here because we just want to log error and exit
     *        all of this is technically before entry so it's fine that we are a little out of bounds
     *        of the "no exceptions" rule since
     **/

    spdlog::sink_ptr stdout_sink_fn(const config_table& config) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
      return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
      return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif
    }

    void initialize_logger(const config_table* config) {
      PROFILE_SECTION("other::register-log-sinks");
      subsystem<logger>::inert = false;

      logger* log = subsystem<logger>::get();
      if (log == nullptr) {
        throw std::runtime_error("Logger subsystem is null.");
      }

      log->set_config(config);
      log->create_logger("other-core-log", spdlog::level::trace);
      // clang-format off
      log_sink sink = {
        1, "console-sink", "%^[%l]%$ %v (%t)", (spdlog::level::level_enum)config->core_log_level, 
        stdout_sink_fn,
      };
      log_sink file_sink = {
        2, "file-sink", "[%Y-%m-%d %H:%M:%S -  %t] [%l] %v", (spdlog::level::level_enum)config->file_log_level,
        [](const config_table& config) -> spdlog::sink_ptr { return std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.core_log_file, true); },
      };
      // clang-format on

      std::string loggers[] = { "other-core-log" };
      log->register_sink(loggers, &sink);
      log->register_sink(loggers, &file_sink);
    }

    void initialize_arena(const config_table* config) {
      PROFILE_SECTION("other::detail::initialize_arena");
      subsystem<arena>::inert = false;

      arena* primary_arena = subsystem<arena>::get();
      if (primary_arena == nullptr) {
        throw std::runtime_error("Primary arena is null.");
      }
      CORE_LOG_DEBUG("Initialized primary arena subsystem.");
    }

    void initialize_file_system(const config_table* config) {
      subsystem<file_system>::inert = false;

      file_system* fs = subsystem<file_system>::get();
      if (fs == nullptr) {
        throw std::runtime_error("File system subsystem is null.");
      }
    }

    void initialize_input_system(const config_table* config) {
      subsystem<input_system>::inert = false;

      input_system* input = subsystem<input_system>::get();
      if (input == nullptr) {
        throw std::runtime_error("Input system subsystem is null.");
      }
    }

    void initialize_type_database(const config_table* config) {
      subsystem<type_database>::inert = false;

      type_database* type_db = subsystem<type_database>::get();
      if (type_db == nullptr) {
        throw std::runtime_error("Type database subsystem is null.");
      }
    }

    void initialize_physics_environment(const config_table* config) {
      subsystem<physics_environment>::inert = false;

      physics_environment* physics_env = subsystem<physics_environment>::get();
      if (physics_env == nullptr) {
        throw std::runtime_error("Physics environment subsystem is null.");
      }

      physics_env->load_backend(*config);
      physics_env->initialize_physics_environment(*config);
    }

    void initialize_renderer_backend(const config_table* config) {
      subsystem<renderer_backend>::inert = false;

      auto* backend = subsystem<renderer_backend>::get();
      if (backend == nullptr) {
        throw std::runtime_error("Renderer backend subsystem is null.");
      }

      backend->load_backend(*config, config->rendering_backend.value(), config->window_size);
    }

    void initialize_scripting_environment(const config_table* config) {
      subsystem<scripting_environment>::inert = false;

      auto* env = subsystem<scripting_environment>::get();
      if (env == nullptr) {
        throw std::runtime_error("scripting_environment null in initialize!");
      }

      {
        PROFILE_SECTION("other::bind-primary-scripting-environment");
        env->initialize_script_environment(*config);

        filepath other_cs_path = config->get_value<std::string>(configuration::kOtherCSharp, "C:/OtherEnvironment/dotnet-assemblies/OtherCs.dll");
        OTHER_ASSERT(std::filesystem::exists(other_cs_path), "OtherCs.dll not found at path: {}", other_cs_path.string());
        CORE_LOG_DEBUG("Using OtherCs.dll at path: {}", other_cs_path.string());
        env->dotnet_binding_assembly = env->load_dotnet_module(other_cs_path.string());
      }

      /// this is the internal scripting interfaces used to glue the script code to the native environment
      ///  this is required for main driver initialization so we load it here
      {
        PROFILE_SECTION("other::bind-environment-scripts");
        dotnet_host& dn_host = env->get_dotnet_host();
        bind_otherlib_dotnet_functions(dn_host);

        lua_host& l_host = env->get_lua_host();
        bind_otherlib_lua_functions(l_host);
      }
    }

    void initialize_audio_environment(const config_table* config) {
      subsystem<audio_environment>::inert = false;

      auto* env = subsystem<audio_environment>::get();
      if (env == nullptr) {
        throw std::runtime_error("Audio environment subsystem is null.");
      }

      audio_config audio_cfg{};
      audio_cfg.sample_rate = config->get_value<uint32_t>("audio.sample-rate", 48000u);
      audio_cfg.force_pump_mode = config->get_value<bool>("audio.force-pump", false);
      env->initialize(audio_cfg);
    }

    void shutdown_logger() {
      CORE_LOG_INFO("Shutting down logger subsystem.");
      subsystem<logger>::get()->shutdown();
    }

    void shutdown_arena() {
    }

    void shutdown_file_system() {
    }

    void shutdown_input_system() {
    }

    void shutdown_type_database() {
    }

    void shutdown_physics_environment() {
      subsystem<physics_environment>::get()->shutdown_physics_environment();
      subsystem<physics_environment>::get()->unload_backend();
    }

    void shutdown_renderer_backend() {
      PROFILE_SECTION("other::detail::shutdown_renderer_backend");
      subsystem<renderer_backend>::get()->unload_backend();
    }

    void shutdown_audio_environment() {
      PROFILE_SECTION("other::detail::shutdown_audio_environment");
      subsystem<audio_environment>::get()->shutdown();
    }

    void shutdown_scripting_environment() {
      PROFILE_SECTION("other::detail::shutdown_scripting_environment");
      subsystem<scripting_environment>::get()->destroy_all_objects();
      subsystem<scripting_environment>::get()->unload_dotnet_module(subsystem<scripting_environment>::get()->dotnet_binding_assembly);
      subsystem<scripting_environment>::get()->dotnet_binding_assembly = nullptr;
      subsystem<scripting_environment>::get()->shutdown_script_environment();
    }

  }  // namespace detail
}  // namespace other