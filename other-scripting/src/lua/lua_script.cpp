/**
 * \file lua/lua_script.cpp
 **/
#include "lua/lua_script.hpp"

#include "core/logger.hpp"

#include "sol/protected_function_result.hpp"

namespace other {

  lua_script::lua_script(sol::state& state, sol::environment&& env, sol::protected_function load_fn)
      : lua_state(state), script_env(std::move(env)), load_fn(load_fn) {
    sol::set_environment(script_env, load_fn);
    if (load_fn.valid()) {
      script_env["__script_loaded"] = true;
    } else {
      script_env["__script_loaded"] = false;
    }
  }

  bool lua_script::is_valid() const {
    return script_env.valid() && script_env["__script_loaded"] == true;
  }

  void lua_script::run_script() {
    if (!is_valid()) {
      CORE_LOG_ERROR("Lua script [{}] is not valid, cannot run script", script_env["__script_name"].get<std::string>());
      return;
    }
    if (ran) {
      return;
    }

    load_fn();
    call_hook_function_if_exists("__other_environment_entry_hook", true);
    ran = true;
  }

  sol::table lua_script::get_symbol_as_table(const std::string_view symbol_name) {
    if (!script_ready()) {
      return sol::table{};
    }

    sol::object symbol = script_env.get<sol::object>(symbol_name);
    if (!symbol.is<sol::table>()) {
      CORE_LOG_ERROR("Symbol '{}' is not a Lua table", symbol_name);
      return sol::table{};
    }

    return symbol.as<sol::table>();
  }

  bool lua_script::has_symbol(const std::string_view symbol_name) {
    if (!script_ready()) {
      return false;
    }

    return script_env[symbol_name].valid();
  }

  void lua_script::call_hook_function_if_exists(const std::string_view function_name, bool on_entry) {
    if (!on_entry) {
      /// if we are entering we already validated and are running the first time
      ///   this will actually also cause infinite recursion if the hook calls this function again
      if (!script_ready()) {
        return;
      }
    }

    try {
      sol::protected_function func = script_env.get<sol::protected_function>(function_name);
      if (!func.valid()) {
        return;
      }

      func();
    } catch (const sol::error& e) {
      CORE_LOG_ERROR("Sol2 error while calling Lua hook function '{}': {}", function_name, e.what());
      return;
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Exception while calling Lua hook function '{}': {}", function_name, e.what());
      return;
    } catch (...) {
      CORE_LOG_ERROR("Unknown error while calling Lua hook function '{}'", function_name);
      return;
    }
  }

}  // namespace other