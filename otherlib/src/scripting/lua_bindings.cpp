/**
 * \file scripting/lua_bindings.cpp
 **/
#include "scripting/lua_bindings.hpp"

#include "core/logger.hpp"

#include "driver/driver.hpp"

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

    lua_state.create_named_table(
      "__other_native",
      "__log", lua_state.create_table_with(),
      "__driver", lua_state.create_table_with()
    );
    sol::table log_table = lua_state["__other_native"]["__log"];
    log_table.set_function("send_log_message", [](spdlog::level::level_enum level, const std::string& message, const std::string& source, int line) {
      other::subsystem<other::logger>::get()->send_log(level, "other-core-log", std::format("{} @ ({}:{})", message, source, line));
    });
  }

  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver) {
    sol::state& lua_state = lua_host.get_lua_state();

    sol::table driver_table = lua_state["__other_native"]["__driver"];
    driver_table.set_function("trigger_driver_event", [host_driver](const std::string& event) {
      host_driver->get_event_system()->trigger_event(event);
    });
  }

}  // namespace other