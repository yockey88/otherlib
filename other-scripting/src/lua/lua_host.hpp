/**
 * \file lua/lua_host.hpp
 **/
#ifndef OTHER_SCRIPTING_LUA_HOST_HPP
#define OTHER_SCRIPTING_LUA_HOST_HPP

#include <string_view>

#include "core/config_table.hpp"

#include "lua/lua_script.hpp"
#include "lua/sol_bridge.hpp"

#include "sol/load_result.hpp"

namespace other {

  class lua_host {
   public:
    lua_host() = default;
    ~lua_host() = default;

    void load_host(const config_table& config);
    void call_entry_point();

    sol::state& get_lua_state() { return lua_state; }
    const sol::state& get_lua_state() const { return lua_state; }

    lua_script* load_file(const std::string_view file_path);

    void shutdown();

   private:
    sol::state lua_state;
    std::map<natural_t, sol::load_result> loaded_scripts;
    std::map<natural_t, lua_script> loaded_lua_scripts;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_LUA_HOST_HPP