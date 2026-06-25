/**
 * \file scripting/lua_bindings/bind_rendering_types_lua.cpp
 **/
#include "scripting/lua_bindings/bind_rendering_types_lua.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/render_component.hpp"


namespace other {

  void bind_rendering_types(sol::state& lua_state) {
    // my_render.material.diffuse_color = glm::vec3(0.4f, 0.6f, 0.8f);
    // my_render.material.diffuse_reflectivity = 0.5f;
    // my_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
    // my_render.material.specular_reflectivity = 0.5f;
    // my_render.material.emissivity = 0.1f;
    // my_render.material.shininess = 16.f;
    // my_render.material.transparency = 0.f;
    lua_state.new_usertype<gpu::graphics_material>(
      "__native_gpu_graphics_material",
      sol::constructors<gpu::graphics_material()>(),
      "diffuse_color", &gpu::graphics_material::diffuse_color,
      "diffuse_reflectivity", &gpu::graphics_material::diffuse_reflectivity,
      "specular_color", &gpu::graphics_material::specular_color,
      "specular_reflectivity", &gpu::graphics_material::specular_reflectivity,
      "emissivity", &gpu::graphics_material::emissivity,
      "shininess", &gpu::graphics_material::shininess,
      "transparency", &gpu::graphics_material::transparency);

    lua_state.new_usertype<point_light>(
      "__native_point_light",
      sol::constructors<point_light()>(),
      "position", &point_light::position,
      "color", &point_light::color);

    lua_state.new_usertype<direction_light>(
      "__native_directional_light",
      sol::constructors<direction_light()>(),
      "direction", &direction_light::direction,
      "color", &direction_light::color);

    lua_state.new_usertype<render_component_lua_proxy>(
      "__native_render_component",
      sol::constructors<render_component_lua_proxy()>(),
      "SetMaterial",
      [](render_component_lua_proxy& self, const gpu::graphics_material& mat) {
        if (self.native_pointer) {
          self.native_pointer->material = mat;
        }
      });

    lua_state.new_usertype<camera_component_lua_proxy>(
      "__native_camera_component",
      "sensitivity",
      sol::property(
        [](camera_component_lua_proxy& self) -> real_t {
          if (self.native_pointer) {
            return self.native_pointer->camera.sensitivity;
          }
          return 0.0f;
        },
        [](camera_component_lua_proxy& self, real_t value) {
        if (self.native_pointer) {
          self.native_pointer->camera.sensitivity = value;
        } }),
      "Look",
      [](camera_component_lua_proxy& self, const glm::vec3& position, const glm::vec3& target) {
        if (self.native_pointer) {
          self.native_pointer->camera.look(position, target);
        }
      },
      "LookFrom",
      [](camera_component_lua_proxy& self, const glm::vec3& position) {
        if (self.native_pointer) {
          self.native_pointer->camera.look_from(position);
        }
      },
      "LookAt",
      [](camera_component_lua_proxy& self, const glm::vec3& target) {
        if (self.native_pointer) {
          self.native_pointer->camera.look_at(target);
        }
      });
  }

}  // namespace other