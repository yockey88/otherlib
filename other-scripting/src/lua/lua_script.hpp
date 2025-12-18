/**
 * \file lua/lua_script.hpp
 **/
#ifndef OTHER_SCRIPTING_LUA_SCRIPT_HPP
#define OTHER_SCRIPTING_LUA_SCRIPT_HPP

#include <type_traits>

#include "core/logger.hpp"

#include "lua/lua_sandbox.hpp"

namespace other {

  class lua_script {
   public:
    lua_script(sol::state& state, sol::environment&& env, sol::protected_function load_result);
    ~lua_script() = default;

    sol::state& get_lua_state() { return lua_state; }
    sol::environment& get_state() { return script_sandbox.environment(); }
    bool is_valid() const;
    void run_script();

    bool script_ready() {
      run_script();
      if (!is_valid()) {
        return false;
      }
      return true;
    }

    sol::table get_symbol_as_table(const std::string_view symbol_name);

    template <typename T>
      requires requires(T t) { T{}; } && (!std::is_pointer_v<T> && !std::is_function_v<T>)
    T get_symbol_as(const std::string_view symbol_name, T default_value = {}) {
      if (!script_ready()) {
        return T{};
      }

      sol::object symbol = script_sandbox.environment().get_or<sol::object>(symbol_name, sol::nil);
      if (!symbol.is<T>()) {
        CORE_LOG_ERROR("Symbol '{}' is not of the requested type in Lua script", symbol_name);
        return default_value;
      }

      return symbol.as<T>();
    }

    template <typename T>
      requires(std::is_pointer_v<T> && !std::is_function_v<T>)
    T get_symbol_as(const std::string_view symbol_name, T default_value = nullptr) {
      if (!script_ready()) {
        return T{};
      }

      sol::object symbol = script_sandbox.environment().get_or<sol::object>(symbol_name, sol::nil);
      if (!symbol.is<T>()) {
        CORE_LOG_ERROR("Symbol '{}' is not of the requested type in Lua script", symbol_name);
        return default_value;
      }

      return symbol.as<T>();
    }

    template <typename Fn, typename... Args>
      requires requires(Fn f) { f(std::declval<Args...>()); }
    Fn get_symbol_as(const std::string_view symbol_name) {
      if (!script_ready()) {
        return nullptr;
      }

      sol::object symbol = script_sandbox.environment().get_or<Fn>(symbol_name, sol::nil);
      if (!symbol.is<sol::protected_function>()) {
        CORE_LOG_ERROR("Symbol '{}' is not a function in Lua script", symbol_name);
        return nullptr;
      }

      return symbol.as<Fn>();
    }

    bool has_symbol(const std::string_view symbol_name);

    template <typename T>
    void add_lua_symbol(const std::string_view name, T&& value) {
      script_sandbox.environment()[name] = std::forward<T>(value);
    }

    void call_hook_function_if_exists(const std::string_view function_name, bool on_entry = false);

    template <typename R, typename... Args>
    R call_function(const std::string_view function_name, Args&&... args) {
      if (!script_ready()) {
        return R{};
      }

      try {
        sol::protected_function func = script_sandbox.environment()[function_name];
        if (!func.valid()) {
          CORE_LOG_ERROR("Function '{}' not found in Lua script", function_name);
          return R{};
        }

        sol::protected_function_result result = func(std::forward<Args>(args)...);
        if (!result.valid()) {
          sol::error err = result;
          CORE_LOG_ERROR("Failed to execute Lua function '{}': {}", function_name, err.what());
          return R{};
        }

        if constexpr (std::is_same_v<R, void>) {
          return;
        } else {
          return result.get<R>();
        }
      } catch (const sol::error& e) {
        CORE_LOG_ERROR("Sol2 error while calling Lua function '{}': {}", function_name, e.what());
        return R{};
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception while calling Lua function '{}': {}", function_name, e.what());
        return R{};
      }
    }

   private:
    sol::state& lua_state;
    lua_sandbox script_sandbox;
    sol::protected_function load_fn;

    bool ran = false;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_LUA_SCRIPT_HPP