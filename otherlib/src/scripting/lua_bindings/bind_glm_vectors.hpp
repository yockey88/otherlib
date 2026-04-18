/**
 * \file scripting/lua_bindings/bind_glm_vectors.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_VECTORS_HPP
#define OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_VECTORS_HPP

#include <sol/sol.hpp>

namespace other {

  void bind_glm_vector_types(sol::state& lua_state);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_VECTORS_HPP