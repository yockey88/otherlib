/**
 * \file scripting/lua_bindings.cpp
 **/
#include "scripting/lua_bindings.hpp"

#include <cstdint>

#include "core/logger.hpp"

#include "driver/driver.hpp"
#include "tools/environment_console.hpp"

namespace other {

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

    lua_state.create_named_table(
      "__other_native",
      "__log", lua_state.create_table_with(),
      "__driver", lua_state.create_table_with(),
      "__environment_console", lua_state.create_table_with()
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

    sol::table console_table = lua_state["__other_native"]["__environment_console"];
    console_table.set_function(
      "submit_console_text",
      [](const std::string& text, console_message_type type) {
        environment_console::submit_console_text(text, type, sys_clock::now());
      }
    );

    CORE_LOG_DEBUG("Loading lua bridge script '{}'.", lua_bridge_path.string());
    lua_state.script_file(lua_bridge_path.string());
  }

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver) {
    sol::state& lua_state = lua_host.get_lua_state();

    sol::table driver_table = lua_state["__other_native"]["__driver"];
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
        default:
          CORE_LOG_WARN("Unsupported data type for event user data: {}", static_cast<int>(data.get_type()));
          break;
      }
      host_driver->trigger_event(event, val);
    });
  }

}  // namespace other