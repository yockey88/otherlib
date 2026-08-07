/**
 * \file scripting/lua_bindings/bind_rendering_types_lua.cpp
 **/
#include "scripting/lua_bindings/bind_rendering_types_lua.hpp"

#include "core/profiler.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/render_component.hpp"

#include "driver/driver.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"

namespace other {

  void bind_rendering_types(sol::state& lua_state) {
    PROFILE_SECTION("bind_rendering_types");
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
      /// materials are assets: scripts assign a .omat path ("" clears back to the model's
      ///  imported materials); parameter poking is a material-asset edit, not a component edit
      "SetMaterial",
      [](render_component_lua_proxy& self, const std::string& path) {
        if (self.native_pointer == nullptr) {
          return;
        }
        if (path.empty()) {
          self.native_pointer->material_asset_id = 0;
          self.native_pointer->last_material_asset_id = 0;
          return;
        }

        driver* d = detail::get_dotnet_native_driver_unchecked();
        if (d == nullptr) {
          CORE_LOG_ERROR("SetMaterial('{}'): no driver bound for script interfaces yet", path);
          return;
        }
        const natural_t material_id = d->begin_asset_load(filepath{ path });
        if (material_id == 0) {
          CORE_LOG_ERROR("SetMaterial('{}'): material could not begin loading", path);
          return;
        }
        self.native_pointer->material_asset_id = material_id;
        self.native_pointer->last_material_asset_id = material_id;
      },
      "SetTint",
      [](render_component_lua_proxy& self, const glm::vec4& tint) {
        if (self.native_pointer) {
          self.native_pointer->tint = tint;
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