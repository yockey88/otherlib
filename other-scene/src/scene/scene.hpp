/**
 * \file scene/scene.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_HPP
#define OTHER_SCENE_SCENE_SCENE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"

#include "scene/scene_tree.hpp"

#include "object/scene_object.hpp"
#include "object/transform.hpp"

namespace other {

  class scene {
   public:
    scene() : tree(this) {}

    scene_object& create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object = nullptr);
    void destroy_object(natural_t id);

    transform& get_transform(scene_object* object);
    void set_transform(scene_object* object, const transform& t);

    transform& get_transform(natural_t id);
    void set_transform(natural_t id, const transform& t);

    template <typename T>
    T& add_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot add component to a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return registry.emplace<T>(entity);
    }

    template <typename T>
    T* get_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot get component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return registry.try_get<T>(entity);
    }

    template <typename T>
    void remove_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot remove component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      registry.remove<T>(entity);
    }

    template <typename T>
    bool has_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot check component on a null scene object.");
      return get_component<T>(object) != nullptr;
    }

   private:
    friend class scene_tree;

    void register_object(scene_object* object, const std::string& name, const glm::vec3& world_position);
    void unregister_object(scene_object* object);

    entt::registry registry;
    scene_tree tree;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_HPP