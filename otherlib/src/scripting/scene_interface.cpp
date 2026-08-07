/**
 * \file scripting/scene_interface.cpp
 **/
#include "scripting/scene_interface.hpp"

#include <filesystem>

#include "core/profiler.hpp"

#include "serialization/reflection.hpp"

#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"

namespace other {

  driver* scene_interface::driver_ptr = nullptr;

  void scene_interface::initialize(driver* host_driver) {
    OTHER_ASSERT(host_driver != nullptr, "Host driver pointer is null in scene_interface::initialize");
    driver_ptr = host_driver;
  }

  void scene_interface::set_scene_clear_color(scene* scene, float r, float g, float b, float a) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::set_scene_clear_color");
    scene->get_storage().clear_color = glm::vec4{ r, g, b, a };
  }

  glm::vec4 scene_interface::get_scene_clear_color(scene* scene) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::get_scene_clear_color");
    return scene->get_storage().clear_color;
  }

  std::string scene_interface::get_object_name(scene* scene, natural_t id) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::get_object_name");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_interface::get_object_name");

    return object->name;
  }

  void scene_interface::set_object_name(scene* scene, natural_t id, const std::string_view name) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::set_object_name");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_interface::set_object_name");

    object->name = name;
  }

  void scene_interface::add_tag_to_object(scene* scene, natural_t id, const std::string_view tag) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::remove_tag_from_object");
    scene->add_object_tag(id, tag);
  }
  void scene_interface::remove_tag_from_object(scene* scene, natural_t id, const std::string_view tag) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::remove_tag_from_object");
    scene->remove_object_tag(id, tag);
  }

  void scene_interface::attach_dotnet_behavior_to_object(scene* scene_ptr, natural_t id, const std::string_view behavior_type_name) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_dotnet_behavior_to_object");
    PROFILE_SECTION("scene_interface::attach_dotnet_behavior_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_dotnet_behavior_to_object");

    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "Scripting environment subsystem is not initialized in scene_object_interface::attach_dotnet_behavior_to_object");

    script_component* script_comp = scene_ptr->get_component<script_component>(object);
    OTHER_ASSERT(script_comp != nullptr, "Script component not found on object [{}] in scene_object_interface::attach_dotnet_behavior_to_object", object->name);
    script_comp->add_behavior(behavior_type_name);
  }

  render_component_lua_proxy scene_interface::attach_model_to_object(scene* scene_ptr, natural_t id, const std::string_view model_path) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_model_to_object");
    OTHER_ASSERT(driver_ptr != nullptr, "Driver pointer is null in scene_object_interface::attach_model_to_object");
    PROFILE_SECTION("scene_interface::attach_model_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_model_to_object");

    render_component* render_comp = nullptr;

    if (!scene_ptr->has_component<render_component>(object)) {
      render_comp = &scene_ptr->add_component<render_component>(object);
    }
    /// this call is from user script so handle errors gracefully
    else {
      render_comp = scene_ptr->get_component<render_component>(object);
    }
    OTHER_ASSERT(render_comp != nullptr, "Render component pointer is null in scene_object_interface::attach_model_to_object");

    filepath path = model_path;
    if (!std::filesystem::exists(path)) {
      path = std::filesystem::absolute(path);
      if (!std::filesystem::exists(path)) {
        CORE_LOG_ERROR("Model file '{}' does not exist", path.string());
        return {};
      }
    }

    CORE_LOG_DEBUG(" [LUA] Beginning asset load for model '{}' to attach to object '{}'.", model_path, object->name);
    render_comp->model_asset_id = driver_ptr->begin_asset_load(model_path);
    render_comp->last_model_asset_id = render_comp->model_asset_id;
    return render_component_lua_proxy{ render_comp };
  }

  camera_component_lua_proxy scene_interface::attach_camera_to_object(scene* scene_ptr, natural_t id) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_camera_to_object");
    PROFILE_SECTION("scene_interface::attach_camera_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_camera_to_object");

    camera_component* camera_comp = nullptr;
    if (!scene_ptr->has_component<camera_component>(object)) {
      camera_comp = &scene_ptr->add_component<camera_component>(object);
    } else {
      camera_comp = scene_ptr->get_component<camera_component>(object);
    }

    OTHER_ASSERT(camera_comp != nullptr, "Camera component pointer is null in scene_object_interface::attach_camera_to_object");
    return camera_component_lua_proxy{ camera_comp };
  }

  point_light scene_interface::attach_point_light_to_object(scene* scene_ptr, natural_t id, const point_light& light) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_light_to_object");
    PROFILE_SECTION("scene_interface::attach_point_light_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_light_to_object");

    point_light_component* light_comp = nullptr;
    if (!scene_ptr->has_component<point_light_component>(object)) {
      light_comp = &scene_ptr->add_component<point_light_component>(object);
    } else {
      light_comp = scene_ptr->get_component<point_light_component>(object);
    }

    OTHER_ASSERT(light_comp != nullptr, "Point light component pointer is null in scene_object_interface::attach_point_light_to_object");
    light_comp->light = light;
    return light_comp->light;
  }

  direction_light scene_interface::attach_direction_light_to_object(scene* scene_ptr, natural_t id, const direction_light& light) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_direction_light_to_object");
    PROFILE_SECTION("scene_interface::attach_direction_light_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_direction_light_to_object");

    direction_light_component* light_comp = nullptr;
    if (!scene_ptr->has_component<direction_light_component>(object)) {
      light_comp = &scene_ptr->add_component<direction_light_component>(object);
    } else {
      light_comp = scene_ptr->get_component<direction_light_component>(object);
    }

    OTHER_ASSERT(light_comp != nullptr, "Direction light component pointer is null in scene_object_interface::attach_direction_light_to_object");
    light_comp->light = light;
    return light_comp->light;
  }

  namespace detail {

    static debug_draw get_scene_interface_draw_sink(driver* drvr, bool in_scene) {
      OTHER_ASSERT(drvr != nullptr, "Driver pointer is null in scene_interface draw function.");
      renderer& r = drvr->get_renderer();
      if (in_scene) {
        return r.scene_overlay();
      }
      /// drivers without a debug view never register the debug streams, drop those draws quietly
      if (r.get_stream_registry().find(builtin_debug_streams::kLines) == nullptr) {
        return debug_draw{ nullptr };
      }
      return r.debug();
    }

  }  // namespace detail

  void scene_interface::draw_line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color, bool in_scene) {
    debug_draw draw = detail::get_scene_interface_draw_sink(driver_ptr, in_scene);
    draw.line(a, b, color);
  }

  void scene_interface::draw_triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec4& color, bool in_scene) {
    debug_draw draw = detail::get_scene_interface_draw_sink(driver_ptr, in_scene);
    draw.triangle(a, b, c, color);
  }

  void scene_interface::draw_point(const glm::vec3& p, const glm::vec4& color, bool in_scene) {
    debug_draw draw = detail::get_scene_interface_draw_sink(driver_ptr, in_scene);
    draw.point(p, color);
  }

}  // namespace other