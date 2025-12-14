/**
 * \file scripting/binding_utils/bind_glm_matrices.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_MATRICES_HPP
#define OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_MATRICES_HPP

#include <sol/sol.hpp>

namespace other {

  void bind_glm_matrix_types(sol::state& lua_state);

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_BINDING_UTILS_BIND_GLM_MATRICES_HPP