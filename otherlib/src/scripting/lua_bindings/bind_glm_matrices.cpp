/**
 * \file scripting/lua_bindings/bind_glm_matrices.cpp
 **/
#include "scripting/lua_bindings/bind_glm_matrices.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/profiler.hpp"

#include "math/definitions.hpp"

namespace other {

  void bind_glm_matrix_types(sol::state& lua_state) {
    PROFILE_SECTION("bind_glm_matrix_types");
    auto mat2_type = lua_state.new_usertype<glm::mat2>(
      "mat2",
      sol::constructors<
        glm::mat2(),
        glm::mat2(float),
        glm::mat2(const glm::vec2&, const glm::vec2&)>()
    );

    mat2_type[sol::meta_function::addition] = [](const glm::mat2& a, const glm::mat2& b) {
      return a + b;
    };
    mat2_type[sol::meta_function::subtraction] = [](const glm::mat2& a, const glm::mat2& b) {
      return a - b;
    };
    mat2_type[sol::meta_function::equal_to] = [](const glm::mat2& a, const glm::mat2& b) {
      return a == b;
    };
    mat2_type[sol::meta_function::multiplication] = sol::overload(
      [](const glm::mat2& a, const glm::mat2& b) { return a * b; },
      [](const glm::mat2& a, const glm::vec2& v) { return a * v; },
      [](const glm::vec2& v, const glm::mat2& a) { return v * a; },
      [](const glm::mat2& a, float s) { return a * s; },
      [](float s, const glm::mat2& a) { return s * a; }
    );
    mat2_type[sol::meta_function::division] = [](const glm::mat2& a, float s) {
      return a / s;
    };

    mat2_type.set_function("transpose", [](const glm::mat2& self) { return glm::transpose(self); });
    mat2_type.set_function("inverse", [](const glm::mat2& self) { return glm::inverse(self); });
    mat2_type.set_function("determinant", [](const glm::mat2& self) { return glm::determinant(self); });

    auto mat3_type = lua_state.new_usertype<glm::mat3>(
      "mat3",
      sol::constructors<
        glm::mat3(),
        glm::mat3(float),
        glm::mat3(const glm::vec3&, const glm::vec3&, const glm::vec3&)>()
    );

    mat3_type[sol::meta_function::addition] = [](const glm::mat3& a, const glm::mat3& b) {
      return a + b;
    };
    mat3_type[sol::meta_function::subtraction] = [](const glm::mat3& a, const glm::mat3& b) {
      return a - b;
    };
    mat3_type[sol::meta_function::equal_to] = [](const glm::mat3& a, const glm::mat3& b) {
      return a == b;
    };
    mat3_type[sol::meta_function::multiplication] = sol::overload(
      [](const glm::mat3& a, const glm::mat3& b) { return a * b; },
      [](const glm::mat3& a, const glm::vec3& v) { return a * v; },
      [](const glm::vec3& v, const glm::mat3& a) { return v * a; },
      [](const glm::mat3& a, float s) { return a * s; },
      [](float s, const glm::mat3& a) { return s * a; }
    );
    mat3_type[sol::meta_function::division] = [](const glm::mat3& a, float s) {
      return a / s;
    };

    mat3_type.set_function("transpose", [](const glm::mat3& self) { return glm::transpose(self); });
    mat3_type.set_function("inverse", [](const glm::mat3& self) { return glm::inverse(self); });
    mat3_type.set_function("determinant", [](const glm::mat3& self) { return glm::determinant(self); });

    auto mat4_type = lua_state.new_usertype<glm::mat4>(
      "mat4",
      sol::constructors<
        glm::mat4(),
        glm::mat4(float),
        glm::mat4(const glm::vec4&, const glm::vec4&, const glm::vec4&, const glm::vec4&)>()
    );

    mat4_type[sol::meta_function::addition] = [](const glm::mat4& a, const glm::mat4& b) {
      return a + b;
    };
    mat4_type[sol::meta_function::subtraction] = [](const glm::mat4& a, const glm::mat4& b) {
      return a - b;
    };
    mat4_type[sol::meta_function::equal_to] = [](const glm::mat4& a, const glm::mat4& b) {
      return a == b;
    };
    mat4_type[sol::meta_function::multiplication] = sol::overload(
      [](const glm::mat4& a, const glm::mat4& b) { return a * b; },
      [](const glm::mat4& a, const glm::vec4& v) { return a * v; },
      [](const glm::vec4& v, const glm::mat4& a) { return v * a; },
      [](const glm::mat4& a, float s) { return a * s; },
      [](float s, const glm::mat4& a) { return s * a; }
    );
    mat4_type[sol::meta_function::division] = [](const glm::mat4& a, float s) {
      return a / s;
    };

    mat4_type.set_function("transpose", [](const glm::mat4& self) { return glm::transpose(self); });
    mat4_type.set_function("inverse", [](const glm::mat4& self) { return glm::inverse(self); });
    mat4_type.set_function("determinant", [](const glm::mat4& self) { return glm::determinant(self); });

    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized.");

    type_db->get_reflection_data<glm::mat2>(glm::mat2{});
    type_db->get_reflection_data<glm::mat3>(glm::mat3{});
    type_db->get_reflection_data<glm::mat4>(glm::mat4{});
  }

}  // namespace other