/**
 * \file scripting/lua_bindings.cpp
 **/
#include "scripting/lua_bindings.hpp"

#include <cstdint>

#include "core/logger.hpp"

#include "dotnet/dotnet_object.hpp"
#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"

#include "driver/driver.hpp"
#include "scripting/binding_utils/bind_math_types_lua.hpp"
#include "scripting/binding_utils/bind_rendering_types_lua.hpp"
#include "scripting/binding_utils/type_binder.hpp"
#include "scripting/scene_interface.hpp"
#include "tools/environment_console.hpp"

#include "lua_bindings.hpp"

namespace other {

  void bind_native_types(sol::state& lua_state);

  void bind_otherlib_lua_functions(lua_host& lua_host) {
    filepath lua_defs_path = lua_host.retrieve_script_path("global_definitions.lua");
    filepath lua_bridge_path = lua_host.retrieve_script_path("other_bridge.lua");
    if (!std::filesystem::exists(lua_defs_path)) {
      CORE_LOG_ERROR("Lua global definitions script '{}' does not exist.", lua_defs_path.string());
      return;
    }

    if (!std::filesystem::exists(lua_bridge_path)) {
      CORE_LOG_ERROR("Lua bridge script '{}' does not exist.", lua_bridge_path.string());
      return;
    }

    sol::state& lua_state = lua_host.get_lua_state();
    bind_native_types(lua_state);

    lua_state.create_named_table(
      "__other_native",
      "__log", lua_state.create_table_with(),
      "__driver", lua_state.create_table_with(),
      "__environment_console", lua_state.create_table_with(),
      "__native_scene", lua_state.create_table_with(),
      "__scene_interface", lua_state.create_table_with(),
      "__component_names", lua_state.create_table_with(),
      "__dotnet_types", lua_state.create_table_with()
    );

    lua_state.create_named_table(
      "__lua_bridge_metadata",
      "__paths", lua_state.create_table_with()
    );

    sol::table paths_table = lua_state["__lua_bridge_metadata"]["__paths"];
    paths_table["script_directory"] = lua_host.get_environment_script_directory().string();
    paths_table["global_definitions"] = lua_defs_path.string();
    paths_table["other_bridge"] = lua_bridge_path.string();

    CORE_LOG_DEBUG("Loading lua global definitions script '{}'.", lua_defs_path.string());
    lua_state.script_file(lua_defs_path.string());

    sol::table log_table = lua_state["__other_native"]["__log"];
    log_table.set_function("send_log_message", [](spdlog::level::level_enum level, const std::string& message, const std::string& source, int line) {
      other::subsystem<other::logger>::get()->send_log(level, 0, std::format(" [Lua] {} @ ({}:{})", message, source, line));
    });

    CORE_LOG_DEBUG("Loading lua bridge script '{}'.", lua_bridge_path.string());
    lua_state.script_file(lua_bridge_path.string());
  }

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver) {
    sol::state& lua_state = lua_host.get_lua_state();

    sol::table driver_table = lua_state["__other_native"]["__driver"];
    sol::table scene_table = lua_state["__other_native"]["__scene_interface"];
    scene_table.set_function("get_scene_clear_color", &scene_interface::get_scene_clear_color);
    scene_table.set_function("set_scene_clear_color", &scene_interface::set_scene_clear_color);
    scene_table.set_function("get_object_name", &scene_interface::get_object_name);
    scene_table.set_function("set_object_name", &scene_interface::set_object_name);
    scene_table.set_function("add_tag_to_object", &scene_interface::add_tag_to_object);
    scene_table.set_function("remove_tag_from_object", &scene_interface::remove_tag_from_object);
    scene_table.set_function("add_component_to_object", &scene_interface::add_component);
    scene_table.set_function("remove_component_from_object", &scene_interface::remove_component);
    scene_table.set_function("check_if_object_has_component", &scene_interface::has_component);
    scene_table.set_function("attach_dotnet_behavior_to_object", &scene_interface::attach_dotnet_behavior_to_object);
    scene_table.set_function("attach_model_to_object", &scene_interface::attach_model_to_object);
    scene_table.set_function("attach_camera_to_object", &scene_interface::attach_camera_to_object);
    scene_table.set_function("attach_point_light_to_object", &scene_interface::attach_point_light_to_object);
    scene_table.set_function("attach_directional_light_to_object", &scene_interface::attach_directional_light_to_object);

    driver_table["__native_pointer"] = reinterpret_cast<std::uintptr_t>(host_driver);
    driver_table.set_function("trigger_driver_event", [host_driver](const std::string& event, sol::object data) {
      value val;
      switch (data.get_type()) {
        case sol::type::nil: break;
        case sol::type::boolean:
          val = value(data.as<bool>());
          break;
        case sol::type::number:
          val = value(data.as<double>());
          break;
        case sol::type::string:
          val = value(data.as<std::string>());
          break;
        case sol::type::table: {
          sol::table tbl = data.as<sol::table>();
          // val = lua_table_to_value(tbl);
          CORE_LOG_DEBUG("Lua table to value conversion not yet implemented for driver event data.");
          return;
        } break;
        default:
          CORE_LOG_WARN("Unsupported data type for event user data: {}", data.get_type());
          break;
      }
      host_driver->get_kernel().get_core_system<event_driver_system>().trigger_event(&host_driver->get_kernel(), event, val);
    });
    driver_table.set_function("process_driver_event", [host_driver](driver_event event) {
      host_driver->process_driver_event(event);
    });

    /// now we bind dotnet types into lua types by asking the dotnet types to write their descriptor tables
    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "scripting_environment is not initialized.");

    auto& dotnet_host = scripting_env->get_dotnet_host();
    type_cache* types = dotnet_host.get_type_cache();
    OTHER_ASSERT(types != nullptr, "dotnet_host type cache is null.");

    for (auto& [type_hash, dotnet_type_ptr] : *types) {
      sol::table type_table = dotnet_type_ptr.create_lua_descriptor(lua_state);
      CORE_LOG_TRACE("Registering .NET type '{}' in Lua .NET type registry", dotnet_type_ptr.full_name());

      lua_state["__other_native"]["__dotnet_types"][dotnet_type_ptr.full_name()] = type_table;
    }
  }

  void bind_native_types(sol::state& lua_state) {
    lua_state.new_enum(
      "log_level",
      "TRACE", spdlog::level::trace,
      "DEBUG", spdlog::level::debug,
      "INFO", spdlog::level::info,
      "WARN", spdlog::level::warn,
      "ERROR", spdlog::level::err,
      "CRITICAL", spdlog::level::critical
    );
    lua_state.new_enum(
      "console_message",
      "CONSOLE_NONE", CONSOLE_MESSAGE_NONE,
      "CONSOLE_MESSAGE", CONSOLE_MESSAGE_MESSAGE,
      "CONSOLE_TRACE", CONSOLE_MESSAGE_TRACE,
      "CONSOLE_DEBUG", CONSOLE_MESSAGE_DEBUG,
      "CONSOLE_INFO", CONSOLE_MESSAGE_INFO,
      "CONSOLE_WARN", CONSOLE_MESSAGE_WARN,
      "CONSOLE_ERROR", CONSOLE_MESSAGE_ERROR,
      "CONSOLE_COMMAND", CONSOLE_MESSAGE_COMMAND
    );
    lua_state.new_enum(
      "driver_state",
      "STOPPED", driver_state::DRIVER_STATE_STOPPED,
      "INITIALIZING", driver_state::DRIVER_STATE_INITIALIZING,
      "RUNNING", driver_state::DRIVER_STATE_RUNNING,
      "PAUSED", driver_state::DRIVER_STATE_PAUSED,
      "SHUTTING_DOWN", driver_state::DRIVER_STATE_SHUTTING_DOWN,
      "NUM_DRIVER_STATES", driver_state::NUM_STATES
    );
    lua_state.new_enum(
      "driver_event",
      "START", driver_event::DRIVER_EVENT_START,
      "READY", driver_event::DRIVER_EVENT_READY,
      "PAUSE", driver_event::DRIVER_EVENT_PAUSE,
      "RESUME", driver_event::DRIVER_EVENT_RESUME,
      "STOP", driver_event::DRIVER_EVENT_STOP,
      "NUM_DRIVER_EVENTS", driver_event::NUM_EVENTS
    );

    bind_linear_algebra_types(lua_state);
    bind_rendering_types(lua_state);

    /// bind scene components (and other native types)
    type_binder<scene_object>::bind_component_lua(lua_state, "__native_scene_object");
    type_binder<transform>::bind_component_lua(lua_state, "__native_transform_component");
    type_binder<script_component>::bind_component_lua(lua_state, "__native_script_component");
    type_binder<render_component>::bind_component_lua(lua_state, "__native_render_component");
    type_binder<camera_component>::bind_component_lua(lua_state, "__native_camera_component");
    type_binder<light_component>::bind_component_lua(lua_state, "__native_light_component");
  }

}  // namespace other