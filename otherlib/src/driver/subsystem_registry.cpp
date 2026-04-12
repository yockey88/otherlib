/**
 * \file driver/subsystem_registry.cpp
 **/
#include "driver/subsystem_registry.hpp"

#include "core/arena.hpp"
#include "core/logger.hpp"
#include "core/subsystem.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"

#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

#include "driver/driver_mounts.hpp"
#include "scripting/dotnet_bindings.hpp"
#include "scripting/lua_bindings.hpp"

namespace other {

  namespace detail {

    void initialize_arena(const config_table* config);
    void initialize_logger(const config_table* config);
    void initialize_file_system(const config_table* config);
    void initialize_input_system(const config_table* config);
    void initialize_type_database(const config_table* config);
    void initialize_physics_environment(const config_table* config);
    void initialize_renderer_backend(const config_table* config);
    void initialize_scripting_environment(const config_table* config);

    void shutdown_arena();
    void shutdown_logger();
    void shutdown_file_system();
    void shutdown_input_system();
    void shutdown_type_database();
    void shutdown_physics_environment();
    void shutdown_renderer_backend();
    void shutdown_scripting_environment();

  }  // namespace detail

  void subsystem_registry::register_subsystem(const subsystem_definition& def) {
    if (registry.find(FNV(def.name)) != registry.end()) {
      CORE_LOG_ERROR("Subsystem with name '{}' is already registered.", def.name);
      return;
    }

    registry.emplace(FNV(def.name), def);
  }

  void subsystem_registry::initialize_profile(const std::string_view profile, const config_table* config) {
    resolve_dependency_list_and_do_initialization(get_required_subsystems_for_profile(profile), config);
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

      /// \note we can throw here because this is a critical error on initialization and there's no reasonable way to recover from it
      if (def.initialize_fn == nullptr) {
        throw std::runtime_error(std::format("Subsystem '{}' does not have an initialization function.", def.name));
      }
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

    return registry;
  }

  const std::string_view get_subsystem_profile(const config_table* config) {
    std::vector<std::string> profile;
    if (config == nullptr) {
      return subsystem_profile::kFullProfileName;
    }

    if (auto prof = config->try_get_value<std::string>("environment.profile"); prof.has_value() && !prof->empty()) {
      return subsystem_profile::kFullProfileName;
    }

    bool rendering_disabled = (config->rendering_backend.has_value() && config->rendering_backend.value() == "headless") || config->force_no_window;
    bool force_disable_physics = config->get_value<bool>("physics.force-disable-physics", false);
    bool force_disable_scripting = config->get_value<bool>("scripting.force-disable-scripting", false);

    if (!rendering_disabled && !force_disable_physics && !force_disable_scripting) {
      return subsystem_profile::kFullProfileName;
    }
    if (rendering_disabled && !force_disable_physics && !force_disable_scripting) {
      return subsystem_profile::kHeadlessProfileName;
    }

    if (!rendering_disabled && force_disable_physics && force_disable_scripting) {
      return subsystem_profile::kRenderingProfileName;
    }
    if (rendering_disabled && !force_disable_physics && force_disable_scripting) {
      return subsystem_profile::kPhysicsProfileName;
    }
    if (!rendering_disabled && !force_disable_physics && force_disable_scripting) {
      return subsystem_profile::kScriptingProfileName;
    }

    return subsystem_profile::kCoreProfileName;
  }

  std::span<const std::string_view> get_required_subsystems_for_profile(const std::string_view profile_name) {
    if (profile_name == subsystem_profile::kFullProfileName) {
      return subsystem_profile::kFullProfile;
    }
    if (profile_name == subsystem_profile::kHeadlessProfileName) {
      return subsystem_profile::kHeadlessProfile;
    }
    if (profile_name == subsystem_profile::kRenderingProfileName) {
      return subsystem_profile::kRenderingProfile;
    }
    if (profile_name == subsystem_profile::kPhysicsProfileName) {
      return subsystem_profile::kPhysicsProfile;
    }
    if (profile_name == subsystem_profile::kScriptingProfileName) {
      return subsystem_profile::kScriptingProfile;
    }
    if (profile_name == subsystem_profile::kCoreProfileName) {
      return subsystem_profile::kCoreOnlyProfile;
    }

    std::println(std::cerr, "Unknown subsystem profile '{}'. Defaulting to full profile.", profile_name);
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
      logger* log = subsystem<logger>::get();
      if (log == nullptr) {
        throw std::runtime_error("Logger subsystem is null.");
      }

      log->set_config(config);
      log->create_logger("other-core-log", spdlog::level::trace);
      // clang-format off
      log_sink sink = {
        1, "console-sink", "%^[%l]%$ %v (%t)",
        (spdlog::level::level_enum)config->core_log_level, stdout_sink_fn,
      };
      log_sink file_sink = {
        2, "file-sink", "[%Y-%m-%d %H:%M:%S -  %t] [%l] %v", spdlog::level::trace,
        [](const config_table& config) -> spdlog::sink_ptr { return std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.core_log_file, true); },
      };
      // clang-format on

      std::string loggers[] = { "other-core-log" };
      log->register_sink(loggers, sink);
      log->register_sink(loggers, file_sink);
    }

    void initialize_arena(const config_table* config) {
      PROFILE_SECTION("other::detail::initialize_arena");
      arena* primary_arena = subsystem<arena>::get();
      if (primary_arena == nullptr) {
        throw std::runtime_error("Primary arena is null.");
      }
    }

    void initialize_file_system(const config_table* config) {
      file_system* fs = subsystem<file_system>::get();
      if (fs == nullptr) {
        throw std::runtime_error("File system subsystem is null.");
      }

      CORE_LOG_DEBUG("Configuring filesystem mounts from configuration");
      const auto md_mnts = config->get_raw("filesystem.mounts");
      if (md_mnts) {
        if (md_mnts.is_array_of_tables()) {
          const auto* mounts_tables = md_mnts.as_array();
          OTHER_ASSERT(mounts_tables != nullptr, "Invalid format for filesystem mounts in configuration. Expected an array of tables.");

          CORE_LOG_DEBUG("Found {} filesystem mount entries in configuration", mounts_tables->size());
          for (const auto& table : *mounts_tables) {
            OTHER_ASSERT(table.is_table(), "Invalid format for filesystem mounts in configuration. Expected an array of tables.");
            const auto* mount_table = table.as_table();
            OTHER_ASSERT(mount_table != nullptr, "Invalid format for filesystem mounts in configuration. Expected an array of tables.");

            auto name_itr = mount_table->find("name");
            auto type_itr = mount_table->find("type");
            auto path_itr = mount_table->find("path");
            if (name_itr == mount_table->end() || type_itr == mount_table->end()) {
              CORE_LOG_ERROR("Invalid format for filesystem mount entry in configuration. Each mount entry must contain 'name' and 'type' fields.");
              continue;
            }
            if (!name_itr->second.is_string() || !type_itr->second.is_string()) {
              CORE_LOG_ERROR("Invalid format for filesystem mount entry in configuration. 'name' and 'type' fields must be strings.");
              continue;
            }

            std::string name = name_itr->second.as_string()->get();
            std::string type = type_itr->second.as_string()->get();
            /// physical is probably going to be the default use case and needs extra checking
            if (type == "physical") {
              if (path_itr == mount_table->end()) {
                CORE_LOG_ERROR("Invalid format for physical filesystem mount entry in configuration. Physical mounts must contain a 'path' field.");
                continue;
              }
              if (!path_itr->second.is_string()) {
                CORE_LOG_ERROR("Invalid format for physical filesystem mount entry in configuration. 'path' field must be a string.");
                continue;
              }
              std::string path_str = path_itr->second.as_string()->get();
              filepath path(path_str);
              if (!std::filesystem::exists(path)) {
                CORE_LOG_ERROR("Filesystem mount path '{}' does not exist. Cannot configure filesystem mount '{}'.", path_str, name);
                continue;
              }

              fs->mount_directory(name, path);
            }
            /// virtual is simpler
            else if (type == "virtual") {
              if (fs->is_mounted(name)) {
                CORE_LOG_WARN("Filesystem mount '{}' is already mounted. Skipping virtual mount.", name);
                continue;
              }

              fs->mount_virtual(name);
            } else {
              CORE_LOG_ERROR("Invalid filesystem mount type '{}' for mount '{}'. Supported types are 'physical' and 'virtual'.", type, name);
            }
          }
        } else {
          CORE_LOG_ERROR("Invalid format for filesystem mounts in configuration. Expected an array of tables.");
        }
      }

      /// defaults
      /// \todo make this more robust
      fs->mount_virtual(driver_mounts::kAssetMount);
      fs->mount_virtual(driver_mounts::kSceneMount);
      fs->mount_virtual(driver_mounts::kScriptMount);
    }

    void initialize_input_system(const config_table* config) {
      input_system* input = subsystem<input_system>::get();
      if (input == nullptr) {
        throw std::runtime_error("Input system subsystem is null.");
      }
    }

    void initialize_type_database(const config_table* config) {
      type_database* type_db = subsystem<type_database>::get();
      if (type_db == nullptr) {
        throw std::runtime_error("Type database subsystem is null.");
      }
    }

    void initialize_physics_environment(const config_table* config) {
      physics_environment* physics_env = subsystem<physics_environment>::get();
      if (physics_env == nullptr) {
        throw std::runtime_error("Physics environment subsystem is null.");
      }

      physics_env->load_backend(*config);
      physics_env->initialize_physics_environment(*config);
    }

    void initialize_renderer_backend(const config_table* config) {
      auto* backend = subsystem<renderer_backend>::get();
      if (backend == nullptr) {
        throw std::runtime_error("Renderer backend subsystem is null.");
      }

      backend->load_backend(*config, config->rendering_backend.value(), config->window_size);
    }

    void initialize_scripting_environment(const config_table* config) {
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

      {
        PROFILE_SECTION("other::bind-environment-scripts");
        dotnet_host& dn_host = env->get_dotnet_host();
        bind_otherlib_dotnet_functions(dn_host);

        lua_host& l_host = env->get_lua_host();
        bind_otherlib_lua_functions(l_host);
      }
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

    void shutdown_scripting_environment() {
      PROFILE_SECTION("other::detail::shutdown_scripting_environment");
      subsystem<scripting_environment>::get()->unload_dotnet_module(subsystem<scripting_environment>::get()->dotnet_binding_assembly);
      subsystem<scripting_environment>::get()->dotnet_binding_assembly = nullptr;
      subsystem<scripting_environment>::get()->shutdown_script_environment();
    }

  }  // namespace detail
}  // namespace other