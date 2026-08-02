/**
 * \file scripting/scene_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP

#include "lua/lua_host.hpp"

#include "object/animation_controller.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"

namespace other {

  class driver;

  struct scene_object;
  class scene;

  class scene_interface {
   public:
    static void initialize(driver* host_driver);

    static void set_scene_clear_color(scene* scene, float r, float g, float b, float a);
    static glm::vec4 get_scene_clear_color(scene* scene);

    static std::string get_object_name(scene* scene, natural_t id);
    static void set_object_name(scene* scene, natural_t id, const std::string_view name);

    static void add_tag_to_object(scene* scene, natural_t id, const std::string_view tag);
    static void remove_tag_from_object(scene* scene, natural_t id, const std::string_view tag);

    /// lua
    /// \todo .NET
    static void attach_dotnet_behavior_to_object(scene* scene_ptr, natural_t id, const std::string_view behavior_type_name);

    static render_component_lua_proxy attach_model_to_object(scene* scene, natural_t id, const std::string_view model_path);
    static camera_component_lua_proxy attach_camera_to_object(scene* scene_ptr, natural_t id);

    static point_light attach_point_light_to_object(scene* scene_ptr, natural_t id, const point_light& light);
    static direction_light attach_direction_light_to_object(scene* scene_ptr, natural_t id, const direction_light& light);

    /// immediate-mode draws over the renderer draw streams, valid every frame regardless
    ///   of scene playback; in_scene targets the depth-tested scene overlay, otherwise the
    ///   editor debug overlay (dropped quietly when the driver has no debug view)
    static void draw_line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color, bool in_scene);
    static void draw_triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec4& color, bool in_scene);
    static void draw_point(const glm::vec3& p, const glm::vec4& color, bool in_scene);

   private:
    static driver* driver_ptr;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP