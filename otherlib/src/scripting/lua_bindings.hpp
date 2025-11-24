/**
 * \file scripting/lua_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_LUA_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_LUA_BINDINGS_HPP

#include "lua/lua_host.hpp"
#include "lua/sol_bridge.hpp"

namespace other {

  class driver;

  void bind_otherlib_lua_functions(lua_host& lua_host);
  void bind_otherlib_driver_lua_functions(lua_host& lua_host, driver* host_driver);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_LUA_BINDINGS_HPP