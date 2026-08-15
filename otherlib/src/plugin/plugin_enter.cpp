/**
 * \file plugin/plugin_enter.cpp
 *
 * the plugin-side half of the handshake: these objects link into every plugin image, so
 *  nothing here may reference the tracy-touching host backend TU — the profiler arrives
 *  as a table pointer through the argv, exactly like the subsystems do
 **/
#include "plugin/plugin.hpp"

#include "core/logger.hpp"
#include "file/filesystem.hpp"
#include "input/input_system.hpp"
#include "memory/arena.hpp"
#include "serialization/reflection.hpp"
#include "thread/thread_safety.hpp"

#include "physics/physics_environment.hpp"
#include "renderer/renderer_backend.hpp"
#include "script/scripting_environment.hpp"

namespace other {

  void plugin::on_enter(const std::string_view pl_name, other_plugin_argv* argv) {
    /// install before the first zone so the handshake itself records into the host client
    profiling::install_backend(argv->profiler);
    PROFILE_SECTION("plugin::on_enter");
    register_main_thread();
    set_subsystem_flags(pl_name, argv);
    plugin_binding(pl_name, argv);
  }

  void plugin::set_subsystem_flags(const std::string_view pl_name, other_plugin_argv* argv) {
    subsystem<arena>::inert = argv->arena == nullptr;
    subsystem<logger>::inert = argv->logger == nullptr;
    subsystem<file_system>::inert = argv->file_system == nullptr;
    subsystem<input_system>::inert = argv->input_system == nullptr;
    subsystem<type_database>::inert = argv->type_database == nullptr;
    subsystem<physics_environment>::inert = argv->physics_environment == nullptr;
    subsystem<renderer_backend>::inert = argv->renderer == nullptr;
    subsystem<scripting_environment>::inert = argv->scripting_environment == nullptr;
  }

  void plugin::plugin_binding(const std::string_view pl_name, other_plugin_argv* argv) {
    subsystem<arena>::set(argv->arena);
    subsystem<logger>::set(argv->logger);
    subsystem<file_system>::set(argv->file_system);
    subsystem<input_system>::set(argv->input_system);
    subsystem<type_database>::set(argv->type_database);
    subsystem<physics_environment>::set(argv->physics_environment);
    subsystem<renderer_backend>::set(argv->renderer);
    subsystem<scripting_environment>::set(argv->scripting_environment);

    std::stringstream ss;
    ss << "Plugin '" << pl_name << "' bound to subsystems: \n";
    ss << std::format("arena={:p}\n", static_cast<void*>(argv->arena));
    ss << std::format("logger={:p}\n", static_cast<void*>(argv->logger));
    ss << std::format("file_system={:p}\n", static_cast<void*>(argv->file_system));
    ss << std::format("input_system={:p}\n", static_cast<void*>(argv->input_system));
    ss << std::format("type_database={:p}\n", static_cast<void*>(argv->type_database));
    ss << std::format("physics_environment={:p}\n", static_cast<void*>(argv->physics_environment));
    ss << std::format("renderer_backend={:p}\n", static_cast<void*>(argv->renderer));
    ss << std::format("scripting_environment={:p}\n", static_cast<void*>(argv->scripting_environment));
    ss << std::format("profiler={:p}", static_cast<const void*>(argv->profiler));
    CORE_LOG_DEBUG("Plugin '{}' initialized with subsystems: {}", pl_name, ss.str());
  }

}  // namespace other
