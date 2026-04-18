/**
 * \file scripting/lua_bindings/bind_rendering_types_lua.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_RENDERING_TYPES_LUA_HPP
#define OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_RENDERING_TYPES_LUA_HPP

#include <sol/sol.hpp>

namespace other {

  void bind_rendering_types(sol::state& lua_state);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_RENDERING_TYPES_LUA_HPP