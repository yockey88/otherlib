/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include "object/transform.hpp"

namespace other {

  scene_object& scene::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    return tree.create_object(name, world_position, parent_object);
  }

  void scene::destroy_object(natural_t id) {
    tree.destroy_object(id);
  }

  transform& scene::get_transform(scene_object* object) {
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    transform* t = registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  void scene::set_transform(scene_object* object, const transform& t) {
    OTHER_ASSERT(object != nullptr, "Cannot set transform on a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    registry.replace<transform>(entity, t);
  }

  transform& scene::get_transform(natural_t id) {
    scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    return get_transform(node->object);
  }

  void scene::set_transform(natural_t id, const transform& t) {
    scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    set_transform(node->object, t);
  }

  void scene::register_object(scene_object* object, const std::string& name, const glm::vec3& world_position) {
    OTHER_ASSERT(object != nullptr, "Cannot register a null scene object.");

    entt::entity entity = registry.create();
    transform& t = registry.emplace<transform>(entity);
    t = {
      .local_basis = orthonormal_basis(glm::vec3(0, 1, 0)),
      .local_position = world_position,
      .local_scale = glm::vec3(1, 1, 1),
      .local_rotation_quat = glm::quat(1, 0, 0, 0),
    };

    object->name = name;
    object->registry_id = (uint32_t)entity;
  }

  void scene::unregister_object(scene_object* object) {
    OTHER_ASSERT(object != nullptr, "Cannot unregister a null scene object.");
    if (object->registry_id != 0) {
      registry.destroy(entt::entity(object->registry_id));
    }
  }

}  // namespace other