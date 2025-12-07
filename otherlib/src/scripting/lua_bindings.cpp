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
    sol::table log_table = lua_state["__other_native"]["__log"];
    log_table.set_function("send_log_message", [](spdlog::level::level_enum level, const std::string& message, const std::string& source, int line) {
      other::subsystem<other::logger>::get()->send_log(level, 0, std::format("{} @ ({}:{})", message, source, line));
    });

    sol::table console_table = lua_state["__other_native"]["__environment_console"];
    console_table.set_function(
      "submit_console_text",
      [](const std::string& text, console_message_type type) {
        environment_console::submit_console_text(text, type, sys_clock::now());
      }
    );

    /// add a little sugar for the lua side of the bridge
    lua_state.script(R"(
function send_log(level, message)
  --- get source and line from stack of where log_xxx(...) was called (up two levels)
  local info = debug.getinfo(3, "Sl") or {}
  local source = info.short_src or "unknown"
  local line = info.currentline or 0
  __other_native.__log.send_log_message(level, message, source, line)

  --- allow users to define this hook to intercept log messages
  if (__other_log_intercept_hook ~= nil) 
  then
    __other_log_intercept_hook(string.format("[%s] %s:%d: %s", level, source, line, message))
  end
end

CoreLog = {
  log_trace = function(...)    send_log(log_level.TRACE, ...)    end,
  log_debug = function(...)    send_log(log_level.DEBUG, ...)    end,
  log_info = function(...)     send_log(log_level.INFO, ...)     end,
  log_warning = function(...)  send_log(log_level.WARNING, ...)  end,
  log_error = function(...)    send_log(log_level.ERROR, ...)    end,
  log_critical = function(...) send_log(log_level.CRITICAL, ...) end
}
)");
  }

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver) {
    sol::state& lua_state = lua_host.get_lua_state();

    sol::table driver_table = lua_state["__other_native"]["__driver"];
    driver_table["__native_pointer"] = reinterpret_cast<std::uintptr_t>(host_driver);
    driver_table.set_function("trigger_driver_event", [host_driver](const std::string& event) {
      host_driver->get_event_system()->trigger_event(event);
    });
    driver_table.set_function("set_event_user_data", [host_driver](const std::string& event, sol::object data) {
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
      host_driver->get_event_system()->set_user_data(event, val);
    });
  }

}  // namespace other