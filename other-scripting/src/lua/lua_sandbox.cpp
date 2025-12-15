/**
 * \file lua/lua_sandbox.cpp
 **/
#include "lua/lua_sandbox.hpp"

#include "lua/lua_host.hpp"
#include "script/scripting_environment.hpp"

#include "sol/protected_function_result.hpp"
#include "sol/table.hpp"

namespace other {

  void lua_sandbox::script(const std::string_view code) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized in lua_sandbox::script");

    lua_host& lua_host = env->get_lua_host();
    sol::state& lua_state = lua_host.get_lua_state();

    sol::protected_function_result result = lua_state.script(code, environment());
    if (!result.valid()) {
      CORE_LOG_ERROR("Failed to execute Lua code in sandbox.");
      sol::error err = result;
      CORE_LOG_ERROR("Lua Error: {}", err.what());
    }
  }

  sol::table lua_sandbox::try_load_table(lua_host* lua_host, const filepath& path) {
    OTHER_ASSERT(lua_host != nullptr, "Lua host is null in lua_sandbox::try_load_table");
    sol::state& lua_state = lua_host->get_lua_state();
    sol::protected_function_result result_table = lua_state.script_file(path.string(), environment());
    if (!result_table.valid()) {
      CORE_LOG_ERROR("Failed to load Lua table from file '{}'", path.string());
      return sol::table{};
    }

    sol::object result = result_table;
    if (result.get_type() != sol::type::table) {
      CORE_LOG_ERROR("Lua file '{}' did not return a table as expected.", path.string());
      CORE_LOG_WARN("Make sure your Lua file returns a table object!");
      return sol::table{};
    }

    return result.as<sol::table>();
  }

}  // namespace other