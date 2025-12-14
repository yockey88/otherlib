/**
 * \file scripting/binding_utils/bind_glm_vectors.cpp
 **/
#include "scripting/binding_utils/bind_glm_vectors.hpp"

#include <glm/glm.hpp>

#include "math/definitions.hpp"

#include "glm/fwd.hpp"

namespace other {

  void bind_glm_vector_types(sol::state& lua_state) {
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

    // auto ivec2_type = lua_state.new_usertype<glm::ivec2>(
    //   "ivec2",
    //   sol::constructors<
    //     glm::ivec2(),
    //     glm::ivec2(int32_t),
    //     glm::ivec2(int32_t, int32_t)>(),
    //   "x", &glm::ivec2::x,
    //   "y", &glm::ivec2::y
    // );

    // ivec2_type[sol::meta_function::addition] = [](const glm::ivec2& a, const glm::ivec2& b) {
    //   return a + b;
    // };
    // ivec2_type[sol::meta_function::subtraction] = [](const glm::ivec2& a, const glm::ivec2& b) {
    //   return a - b;
    // };
    // ivec2_type[sol::meta_function::unary_minus] = [](const glm::ivec2& a) {
    //   return -a;
    // };
    // ivec2_type[sol::meta_function::equal_to] = [](const glm::ivec2& a, const glm::ivec2& b) {
    //   return a == b;
    // };
    // ivec2_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::ivec2& a, const glm::ivec2& b) { return a * b; },
    //   [](const glm::ivec2& a, int32_t s) { return a * s; },
    //   [](int32_t s, const glm::ivec2& a) { return s * a; }
    // );
    // ivec2_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::ivec2& a, const glm::ivec2& b) { return a / b; },
    //   [](const glm::ivec2& a, int32_t s) { return a / s; }
    // );

    // ivec2_type.set_function("dot", [](const glm::ivec2& self, const glm::ivec2& other) { return self.x * other.x + self.y * other.y; });

    // auto uvec2_type = lua_state.new_usertype<glm::uvec2>(
    //   "uvec2",
    //   sol::constructors<
    //     glm::uvec2(),
    //     glm::uvec2(uint32_t),
    //     glm::uvec2(uint32_t, uint32_t)>(),
    //   "x", &glm::uvec2::x,
    //   "y", &glm::uvec2::y
    // );

    // uvec2_type[sol::meta_function::addition] = [](const glm::uvec2& a, const glm::uvec2& b) {
    //   return a + b;
    // };
    // uvec2_type[sol::meta_function::subtraction] = [](const glm::uvec2& a, const glm::uvec2& b) {
    //   return a - b;
    // };
    // uvec2_type[sol::meta_function::equal_to] = [](const glm::uvec2& a, const glm::uvec2& b) {
    //   return a == b;
    // };
    // uvec2_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::uvec2& a, const glm::uvec2& b) { return a * b; },
    //   [](const glm::uvec2& a, uint32_t s) { return a * s; },
    //   [](uint32_t s, const glm::uvec2& a) { return s * a; }
    // );
    // uvec2_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::uvec2& a, const glm::uvec2& b) { return a / b; },
    //   [](const glm::uvec2& a, uint32_t s) { return a / s; }
    // );

    // uvec2_type.set_function("dot", [](const glm::uvec2& self, const glm::uvec2& other) { return self.x * other.x + self.y * other.y; });

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

    // auto ivec3_type = lua_state.new_usertype<glm::ivec3>(
    //   "ivec3",
    //   sol::constructors<
    //     glm::ivec3(),
    //     glm::ivec3(int32_t),
    //     glm::ivec3(int32_t, int32_t, int32_t)>(),
    //   "x", &glm::ivec3::x,
    //   "y", &glm::ivec3::y,
    //   "z", &glm::ivec3::z
    // );

    // ivec3_type[sol::meta_function::addition] = [](const glm::ivec3& a, const glm::ivec3& b) {
    //   return a + b;
    // };
    // ivec3_type[sol::meta_function::subtraction] = [](const glm::ivec3& a, const glm::ivec3& b) {
    //   return a - b;
    // };
    // ivec3_type[sol::meta_function::unary_minus] = [](const glm::ivec3& a) {
    //   return -a;
    // };
    // ivec3_type[sol::meta_function::equal_to] = [](const glm::ivec3& a, const glm::ivec3& b) {
    //   return a == b;
    // };
    // ivec3_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::ivec3& a, const glm::ivec3& b) { return a * b; },
    //   [](const glm::ivec3& a, int32_t s) { return a * s; },
    //   [](int32_t s, const glm::ivec3& a) { return s * a; }
    // );
    // ivec3_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::ivec3& a, const glm::ivec3& b) { return a / b; },
    //   [](const glm::ivec3& a, int32_t s) { return a / s; }
    // );

    // ivec3_type.set_function("dot", [](const glm::ivec3& self, const glm::ivec3& other) { return self.x * other.x + self.y * other.y + self.z * other.z; });
    // ivec3_type.set_function("cross", [](const glm::ivec3& self, const glm::ivec3& other) {
    //   glm::vec3 self_f(self);
    //   glm::vec3 other_f(other);
    //   glm::vec3 cross_f = glm::cross(self_f, other_f);
    //   return glm::ivec3(static_cast<int32_t>(cross_f.x), static_cast<int32_t>(cross_f.y), static_cast<int32_t>(cross_f.z));
    // });

    // auto uvec3_type = lua_state.new_usertype<glm::uvec3>(
    //   "uvec3",
    //   sol::constructors<
    //     glm::uvec3(),
    //     glm::uvec3(uint32_t),
    //     glm::uvec3(uint32_t, uint32_t, uint32_t)>(),
    //   "x", &glm::uvec3::x,
    //   "y", &glm::uvec3::y,
    //   "z", &glm::uvec3::z
    // );

    // uvec3_type[sol::meta_function::addition] = [](const glm::uvec3& a, const glm::uvec3& b) {
    //   return a + b;
    // };
    // uvec3_type[sol::meta_function::subtraction] = [](const glm::uvec3& a, const glm::uvec3& b) {
    //   return a - b;
    // };
    // uvec3_type[sol::meta_function::equal_to] = [](const glm::uvec3& a, const glm::uvec3& b) {
    //   return a == b;
    // };
    // uvec3_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::uvec3& a, const glm::uvec3& b) { return a * b; },
    //   [](const glm::uvec3& a, uint32_t s) { return a * s; },
    //   [](uint32_t s, const glm::uvec3& a) { return s * a; }
    // );
    // uvec3_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::uvec3& a, const glm::uvec3& b) { return a / b; },
    //   [](const glm::uvec3& a, uint32_t s) { return a / s; }
    // );

    // uvec3_type.set_function("dot", [](const glm::uvec3& self, const glm::uvec3& other) { return self.x * other.x + self.y * other.y + self.z * other.z; });
    // uvec3_type.set_function("cross", [](const glm::uvec3& self, const glm::uvec3& other) {
    //   glm::vec3 self_f(static_cast<float>(self.x), static_cast<float>(self.y), static_cast<float>(self.z));
    //   glm::vec3 other_f(static_cast<float>(other.x), static_cast<float>(other.y), static_cast<float>(other.z));
    //   glm::vec3 cross_f = glm::cross(self_f, other_f);
    //   return glm::uvec3(static_cast<uint32_t>(cross_f.x), static_cast<uint32_t>(cross_f.y), static_cast<uint32_t>(cross_f.z));
    // });

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

    // auto ivec4_type = lua_state.new_usertype<glm::ivec4>(
    //   "ivec4",
    //   sol::constructors<
    //     glm::ivec4(),
    //     glm::ivec4(int32_t),
    //     glm::ivec4(int32_t, int32_t, int32_t, int32_t)>(),
    //   "x", &glm::ivec4::x,
    //   "y", &glm::ivec4::y,
    //   "z", &glm::ivec4::z,
    //   "w", &glm::ivec4::w
    // );

    // ivec4_type[sol::meta_function::addition] = [](const glm::ivec4& a, const glm::ivec4& b) {
    //   return a + b;
    // };
    // ivec4_type[sol::meta_function::subtraction] = [](const glm::ivec4& a, const glm::ivec4& b) {
    //   return a - b;
    // };
    // ivec4_type[sol::meta_function::unary_minus] = [](const glm::ivec4& a) {
    //   return -a;
    // };
    // ivec4_type[sol::meta_function::equal_to] = [](const glm::ivec4& a, const glm::ivec4& b) {
    //   return a == b;
    // };
    // ivec4_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::ivec4& a, const glm::ivec4& b) { return a * b; },
    //   [](const glm::ivec4& a, int32_t s) { return a * s; },
    //   [](int32_t s, const glm::ivec4& a) { return s * a; }
    // );
    // ivec4_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::ivec4& a, const glm::ivec4& b) { return a / b; },
    //   [](const glm::ivec4& a, int32_t s) { return a / s; }
    // );

    // ivec4_type.set_function("dot", [](const glm::ivec4& self, const glm::ivec4& other) { return self.x * other.x + self.y * other.y + self.z * other.z + self.w * other.w; });

    // auto uvec4_type = lua_state.new_usertype<glm::uvec4>(
    //   "uvec4",
    //   sol::constructors<
    //     glm::uvec4(),
    //     glm::uvec4(uint32_t),
    //     glm::uvec4(uint32_t, uint32_t, uint32_t, uint32_t)>(),
    //   "x", &glm::uvec4::x,
    //   "y", &glm::uvec4::y,
    //   "z", &glm::uvec4::z,
    //   "w", &glm::uvec4::w
    // );

    // uvec4_type[sol::meta_function::addition] = [](const glm::uvec4& a, const glm::uvec4& b) {
    //   return a + b;
    // };
    // uvec4_type[sol::meta_function::subtraction] = [](const glm::uvec4& a, const glm::uvec4& b) {
    //   return a - b;
    // };
    // uvec4_type[sol::meta_function::equal_to] = [](const glm::uvec4& a, const glm::uvec4& b) {
    //   return a == b;
    // };
    // uvec4_type[sol::meta_function::multiplication] = sol::overload(
    //   [](const glm::uvec4& a, const glm::uvec4& b) { return a * b; },
    //   [](const glm::uvec4& a, uint32_t s) { return a * s; },
    //   [](uint32_t s, const glm::uvec4& a) { return s * a; }
    // );
    // uvec4_type[sol::meta_function::division] = sol::overload(
    //   [](const glm::uvec4& a, const glm::uvec4& b) { return a / b; },
    //   [](const glm::uvec4& a, uint32_t s) { return a / s; }
    // );

    // uvec4_type.set_function("dot", [](const glm::uvec4& self, const glm::uvec4& other) { return self.x * other.x + self.y * other.y + self.z * other.z + self.w * other.w; });
  }

}  // namespace other