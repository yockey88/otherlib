/**
 * \file scripting/lua_bindings/bind_glm_vectors.cpp
 **/
#include "scripting/lua_bindings/bind_glm_vectors.hpp"

#include <glm/glm.hpp>

#include "core/profiler.hpp"

#include "math/definitions.hpp"

#include "glm/fwd.hpp"

namespace other {

  void bind_glm_vector_types(sol::state& lua_state) {
    PROFILE_SECTION("bind_glm_vector_types");
    auto vec2_type = lua_state.new_usertype<glm::vec2>(
      "__native_vector2",
      sol::constructors<
        glm::vec2(),
        glm::vec2(float),
        glm::vec2(float, float)>(),
      sol::meta_function::addition, [](const glm::vec2& a, const glm::vec2& b) { return a + b; },
      sol::meta_function::subtraction, [](const glm::vec2& a, const glm::vec2& b) { return a - b; },
      sol::meta_function::unary_minus, [](const glm::vec2& a) { return -a; },
      sol::meta_function::equal_to, [](const glm::vec2& a, const glm::vec2& b) { return a == b; },
      sol::meta_function::multiplication,
      sol::overload(
        [](const glm::vec2& a, const glm::vec2& b) { return a * b; },
        [](const glm::vec2& a, float s) { return a * s; },
        [](float s, const glm::vec2& a) { return s * a; }
      ),
      sol::meta_function::division,
      sol::overload(
        [](const glm::vec2& a, const glm::vec2& b) { return a / b; },
        [](const glm::vec2& a, float s) { return a / s; }
      ),
      "x", &glm::vec2::x,
      "y", &glm::vec2::y,
      "dot", [](const glm::vec2& self, const glm::vec2& other) { return glm::dot(self, other); },
      "length", [](const glm::vec2& self) { return glm::length(self); },
      "length_squared", [](const glm::vec2& self) { return glm::dot(self, self); },
      "distance", [](const glm::vec2& self, const glm::vec2& other) { return glm::distance(self, other); },
      "normalized", [](const glm::vec2& self) { return glm::normalize(self); }
    );

    auto vec3_type = lua_state.new_usertype<glm::vec3>(
      "__native_vector3",
      sol::constructors<
        glm::vec3(),
        glm::vec3(float),
        glm::vec3(float, float, float)>(),
      sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
      sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
      sol::meta_function::unary_minus, [](const glm::vec3& a) { return -a; },
      sol::meta_function::equal_to, [](const glm::vec3& a, const glm::vec3& b) { return a == b; },
      sol::meta_function::multiplication,
      sol::overload(
        [](const glm::vec3& a, const glm::vec3& b) { return a * b; },
        [](const glm::vec3& a, float s) { return a * s; },
        [](float s, const glm::vec3& a) { return s * a; }
      ),
      sol::meta_function::division,
      sol::overload(
        [](const glm::vec3& a, const glm::vec3& b) { return a / b; },
        [](const glm::vec3& a, float s) { return a / s; }
      ),
      "x", &glm::vec3::x,
      "y", &glm::vec3::y,
      "z", &glm::vec3::z,
      "dot", [](const glm::vec3& self, const glm::vec3& other) { return glm::dot(self, other); },
      "cross", [](const glm::vec3& self, const glm::vec3& other) { return glm::cross(self, other); },
      "length", [](const glm::vec3& self) { return glm::length(self); },
      "length_squared", [](const glm::vec3& self) { return glm::dot(self, self); },
      "distance", [](const glm::vec3& self, const glm::vec3& other) { return glm::distance(self, other); },
      "normalized", [](const glm::vec3& self) { return glm::normalize(self); }
    );

    auto vec4_type = lua_state.new_usertype<glm::vec4>(
      "__native_vector4",
      sol::constructors<
        glm::vec4(),
        glm::vec4(float),
        glm::vec4(float, float, float, float)>(),
      sol::meta_function::addition, [](const glm::vec4& a, const glm::vec4& b) { return a + b; },
      sol::meta_function::subtraction, [](const glm::vec4& a, const glm::vec4& b) { return a - b; },
      sol::meta_function::unary_minus, [](const glm::vec4& a) { return -a; },
      sol::meta_function::equal_to, [](const glm::vec4& a, const glm::vec4& b) { return a == b; },
      sol::meta_function::multiplication,
      sol::overload(
        [](const glm::vec4& a, const glm::vec4& b) { return a * b; },
        [](const glm::vec4& a, float s) { return a * s; },
        [](float s, const glm::vec4& a) { return s * a; }
      ),
      sol::meta_function::division,
      sol::overload(
        [](const glm::vec4& a, const glm::vec4& b) { return a / b; },
        [](const glm::vec4& a, float s) { return a / s; }
      ),
      "x", &glm::vec4::x,
      "y", &glm::vec4::y,
      "z", &glm::vec4::z,
      "w", &glm::vec4::w,
      "dot", [](const glm::vec4& self, const glm::vec4& other) { return glm::dot(self, other); },
      "length", [](const glm::vec4& self) { return glm::length(self); },
      "length_squared", [](const glm::vec4& self) { return glm::dot(self, self); },
      "distance", [](const glm::vec4& self, const glm::vec4& other) { return glm::distance(self, other); },
      "normalized", [](const glm::vec4& self) { return glm::normalize(self); }
    );

    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized.");

    type_db->get_reflection_data<glm::vec2>(glm::vec2{});
    type_db->get_reflection_data<glm::vec3>(glm::vec3{});
    type_db->get_reflection_data<glm::vec4>(glm::vec4{});
  }

}  // namespace other