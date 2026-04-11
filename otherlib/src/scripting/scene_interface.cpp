/**
 * \file scripting/scene_interface.cpp
 **/
#include "scripting/scene_interface.hpp"

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

  void scene_interface::add_component(scene* scene, natural_t id, const std::string_view name) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_object_interface::add_component");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::add_component");

    /// look name up in type database
    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized in scene_object_interface::add_component");

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Failed to retrieve scripting environment in scene_object_interface::add_component");

    if (type_db->has_type(name)) {
      CORE_LOG_INFO("Added component type [{}] to object [{}] in scene [{}].", name, object->name, scene->name);
      scene->add_component_by_name(object, name);
    } else {
      CORE_LOG_ERROR("Component type [{}] not found in type database.", name);
    }

    /// TODO: fix this, are you kidding me
    sol::environment& lua_env = scene->get_storage().sandbox.environment();
    switch (FNV(name)) {
      case FNV("transform"): {
        transform* comp = scene->get_component<transform>(object);
        OTHER_ASSERT(comp != nullptr, "Failed to retrieve transform component after adding it in scene_object_interface::add_component");
        lua_env["__other_native"]["__local_return_value"] = *comp;
      } break;

      case FNV("script_component"): {
        script_component* comp = scene->get_component<script_component>(object);
        OTHER_ASSERT(comp != nullptr, "Failed to retrieve script component after adding it in scene_object_interface::add_component");
        lua_env["__other_native"]["__local_return_value"] = *comp;
      } break;

      case FNV("render_component"): {
        render_component* comp = scene->get_component<render_component>(object);
        OTHER_ASSERT(comp != nullptr, "Failed to retrieve render component after adding it in scene_object_interface::add_component");
        lua_env["__other_native"]["__local_return_value"] = render_component_lua_proxy{ comp };
      } break;

      case FNV("camera_component"): {
        camera_component* comp = scene->get_component<camera_component>(object);
        OTHER_ASSERT(comp != nullptr, "Failed to retrieve camera component after adding it in scene_object_interface::add_component");
        lua_env["__other_native"]["__local_return_value"] = camera_component_lua_proxy{ comp };
      } break;

      default:
        CORE_LOG_WARN("No Lua binding implemented for component type [{}] added to object [{}] in scene [{}].", name, object->name, scene->name);
        break;
    }
  }

  void scene_interface::remove_component(scene* scene, natural_t id, const std::string_view name) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_object_interface::remove_component");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::remove_component");

    /// look name up in type database
    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized in scene_object_interface::remove_component");

    if (type_db->has_type(name)) {
      CORE_LOG_INFO("Removed component type [{}] from object [{}] in scene [{}].", name, object->name, scene->name);
      scene->remove_component_by_name(object, name);
    } else {
      CORE_LOG_ERROR("Component type [{}] not found in type database.", name);
    }
  }

  bool scene_interface::has_component(scene* scene, natural_t id, const std::string_view name) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_object_interface::has_component");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::has_component");

    /// look name up in type database
    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized in scene_object_interface::has_component");

    if (type_db->has_type(name)) {
      bool has_comp = scene->has_component_by_name(object, name);
      CORE_LOG_INFO("Object [{}] in scene [{}] has component type [{}]: {}.", object->name, scene->name, name, has_comp);
      return has_comp;
    } else {
      CORE_LOG_ERROR("Component type [{}] not found in type database.", name);
      return false;
    }
  }

  void scene_interface::attach_dotnet_behavior_to_object(scene* scene_ptr, natural_t id, const std::string_view behavior_type_name) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_dotnet_behavior_to_object");

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

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_model_to_object");

    render_component* render_comp = nullptr;
    if (!scene_ptr->has_component<render_component>(object)) {
      render_comp = &scene_ptr->add_component<render_component>(object);
    } else {
      render_comp = scene_ptr->get_component<render_component>(object);
    }

    /// this must be validated by script-side of interface
    OTHER_ASSERT(std::filesystem::exists(model_path), "Model path '{}' does not exist in scene_object_interface::attach_model_to_object", model_path);
    OTHER_ASSERT(render_comp != nullptr, "Render component pointer is null in scene_object_interface::attach_model_to_object");

    CORE_LOG_DEBUG(" [LUA] Beginning asset load for model '{}' to attach to object '{}'.", model_path, object->name);
    render_comp->model_asset_id = driver_ptr->begin_asset_load(model_path);
    render_comp->last_model_asset_id = render_comp->model_asset_id;

    return render_component_lua_proxy{ render_comp };
  }

  camera_component_lua_proxy scene_interface::attach_camera_to_object(scene* scene_ptr, natural_t id) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_camera_to_object");

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

  template <typename T>
  T add_light(scene* scene_ptr, natural_t id, const T& light) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null in scene_object_interface::attach_light_to_object");

    scene_object* object = &scene_ptr->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::attach_light_to_object");

    light_component* light_comp = nullptr;
    if (!scene_ptr->has_component<light_component>(object)) {
      light_comp = &scene_ptr->add_component<light_component>(object);
    } else {
      light_comp = scene_ptr->get_component<light_component>(object);
    }

    OTHER_ASSERT(light_comp != nullptr, "Light component pointer is null in scene_object_interface::attach_light_to_object");
    if constexpr (std::is_same_v<T, gpu::point_light>) {
      light_comp->point_lights.push_back(light);
      return light_comp->point_lights.back();
    } else if constexpr (std::is_same_v<T, gpu::directional_light>) {
      light_comp->directional_lights.push_back(light);
      return light_comp->directional_lights.back();
    } else {
      static_assert(false, "Unsupported light type in scene_interface::attach_light_to_object");
    }
  }

  gpu::point_light scene_interface::attach_point_light_to_object(scene* scene_ptr, natural_t id, const gpu::point_light& light) {
    return add_light<gpu::point_light>(scene_ptr, id, light);
  }

  gpu::directional_light scene_interface::attach_directional_light_to_object(scene* scene_ptr, natural_t id, const gpu::directional_light& light) {
    return add_light<gpu::directional_light>(scene_ptr, id, light);
  }

}  // namespace other