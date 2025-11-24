/**
 * \file lua/lua_script.hpp
 **/
#ifndef OTHER_SCRIPTING_LUA_SCRIPT_HPP
#define OTHER_SCRIPTING_LUA_SCRIPT_HPP

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "lua/sol_bridge.hpp"

namespace other {

  class lua_script {
   public:
    lua_script() = default;
    lua_script(sol::state* state)
        : lua_state(state), script_data(nullptr) {}
    lua_script(sol::state* state, sol::load_result* script_data)
        : lua_state(state), script_data(script_data) {}
    ~lua_script() = default;

    bool is_valid() const {
      return script_data != nullptr && script_data->valid();
    }
    void run_script();

    sol::state& get_state() const { return *lua_state; }

    sol::table get_symbol_as_table(const std::string_view symbol_name);

    template <typename R, typename... Args>
    R call_function(const std::string_view function_name, Args&&... args) {
      run_script();
      if (!is_valid()) {
        CORE_LOG_ERROR("Lua script is not valid, cannot call function '{}'", function_name);
        return R{};
      }

      try {
        sol::protected_function func = lua_state->globals().get<sol::protected_function>(function_name);
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
    sol::state* lua_state = nullptr;
    sol::load_result* script_data = nullptr;
    bool ran = false;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_LUA_SCRIPT_HPP