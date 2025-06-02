/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include "object/transform.hpp"

namespace other {

  void scene::register_object(scene_object* object, const std::string& name, const glm::vec3& world_position) {
    OTHER_ASSERT(object != nullptr, "Cannot register a null scene object.");
    object->name = name;
    object->world_position = world_position;

    entt::entity entity = registry.create();
    registry.emplace<transform>(entity);

    object->registry_id = (uint32_t)entity;
  }

  void scene::unregister_object(scene_object* object) {
    OTHER_ASSERT(object != nullptr, "Cannot unregister a null scene object.");
    if (object->registry_id != 0) {
      registry.destroy(entt::entity(object->registry_id));
    }
  }

}  // namespace other