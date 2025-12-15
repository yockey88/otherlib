/**
 * \file scripting/binding_utils/bind_math_types_lua.cpp
 **/
#include "scripting/binding_utils/bind_math_types_lua.hpp"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "math/definitions.hpp"

#include "scripting/binding_utils/bind_glm_matrices.hpp"
#include "scripting/binding_utils/bind_glm_vectors.hpp"

namespace other {

  void bind_linear_algebra_types(sol::state& lua_state) {
    bind_glm_vector_types(lua_state);
    bind_glm_matrix_types(lua_state);

    auto quat_type = lua_state.new_usertype<glm::quat>(
      "__native_quaternion",
      sol::constructors<
        glm::quat(),
        glm::quat(float, float, float, float),
        glm::quat(float, const glm::vec3&),
        glm::quat(const glm::vec3&),
        glm::quat(const glm::vec3&, const glm::vec3&)>(),
      sol::meta_function::addition, [](const glm::quat& self, const glm::quat& other) { return self + other; },
      sol::meta_function::subtraction, [](const glm::quat& self, const glm::quat& other) { return self - other; },
      sol::meta_function::unary_minus, [](const glm::quat& self) { return -self; },
      sol::meta_function::equal_to, [](const glm::quat& self, const glm::quat& other) { return self == other; },
      sol::meta_function::multiplication,
      sol::overload(
        [](const glm::quat& a, const glm::quat& b) { return a * b; },
        [](const glm::quat& q, const glm::vec3& v) { return glm::rotate(q, v); },
        [](const glm::quat& q, float s) { return q * s; },
        [](float s, const glm::quat& q) { return s * q; }
      ),
      sol::meta_function::division, [](const glm::quat& q, float s) { return q / s; },
      "w", &glm::quat::w,
      "x", &glm::quat::x,
      "y", &glm::quat::y,
      "z", &glm::quat::z,
      "dot", [](const glm::quat& self, const glm::quat& other) { return glm::dot(self, other); },
      "length", [](const glm::quat& self) { return glm::length(self); },
      "length_squared", [](const glm::quat& self) { return glm::dot(self, self); },
      "normalized", [](const glm::quat& self) { return glm::normalize(self); },
      "conjugate", [](const glm::quat& self) { return glm::conjugate(self); },
      "inverse", [](const glm::quat& self) { return glm::inverse(self); },
      "slerp", [](const glm::quat& self, const glm::quat& other, float t) { return glm::slerp(self, other, t); },
      "rotate", [](const glm::quat& self, const glm::vec3& v) { return glm::rotate(self, v); }
    );

    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized.");

    type_db->get_reflection_data<glm::quat>(glm::quat{});
  }

}  // namespace other