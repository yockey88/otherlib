/**
 * \file lua/lua_script.cpp
 **/
#include "lua/lua_script.hpp"

#include "core/logger.hpp"

namespace other {

  void lua_script::run_script() {
    if (ran) {
      return;
    }
    if (!is_valid()) {
      CORE_LOG_ERROR("Lua script is not valid, cannot run script.");
      return;
    }

    auto script_result = (*script_data)();
    if (!script_result.valid()) {
      sol::error err = script_result;
      CORE_LOG_ERROR("Failed to execute Lua script: {}", err.what());
      return;
    }
    ran = true;

    auto other_env_entry_hook = lua_state->get_or<std::function<void()>>("other_env_entry_hook", nullptr);
    if (other_env_entry_hook != nullptr) {
      other_env_entry_hook();
    }
  }

  sol::table lua_script::get_symbol_as_table(const std::string_view symbol_name) {
    run_script();
    if (!is_valid()) {
      return sol::table{};
    }

    sol::protected_function_result result = (*script_data)();
    if (!result.valid()) {
      sol::error err = result;
      CORE_LOG_ERROR("Failed to execute Lua script to get symbol '{}': {}", symbol_name, err.what());
      return sol::table{};
    }

    sol::table global_table = lua_state->globals();
    sol::object symbol = global_table.get<sol::object>(symbol_name);
    if (!symbol.is<sol::table>()) {
      CORE_LOG_ERROR("Symbol '{}' is not a Lua table", symbol_name);
      return sol::table{};
    }

    return symbol.as<sol::table>();
  }

}  // namespace other