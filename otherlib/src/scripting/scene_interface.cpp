/**
 * \file scripting/scene_interface.cpp
 **/
#include "scripting/scene_interface.hpp"

#include "serialization/reflection.hpp"

#include "object/scene_object.hpp"
#include "scene/scene.hpp"

namespace other {

  void scene_interface::add_component(scene* scene, natural_t id, const std::string_view name) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_object_interface::add_component");

    scene_object* object = &scene->get_object(id);
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in scene_object_interface::add_component");

    /// look name up in type database
    auto* type_db = subsystem<type_database>::get();
    OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized in scene_object_interface::add_component");

    if (type_db->has_type(name)) {
      CORE_LOG_INFO("Added component type [{}] to object [{}] in scene [{}].", name, object->name, scene->name);
      scene->add_component_by_name(object, name);
    } else {
      CORE_LOG_ERROR("Component type [{}] not found in type database.", name);
    }
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

  void scene_interface::set_scene_clear_color(scene* scene, float r, float g, float b, float a) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::set_scene_clear_color");
    scene->get_storage().clear_color = glm::vec4{ r, g, b, a };
  }

  glm::vec4 scene_interface::get_scene_clear_color(scene* scene) {
    OTHER_ASSERT(scene != nullptr, "Scene pointer is null in scene_interface::get_scene_clear_color");
    return scene->get_storage().clear_color;
  }

}  // namespace other