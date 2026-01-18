/**
 * \file scripting/binding_utils/bind_math_types_lua.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_MATH_TYPES_LUA_HPP
#define OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_MATH_TYPES_LUA_HPP

#include <sol/sol.hpp>

namespace other {

  void bind_linear_algebra_types(sol::state& lua_state);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_MATH_TYPES_LUA_HPP