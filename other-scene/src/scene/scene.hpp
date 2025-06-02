/**
 * \file scene/scene.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_HPP
#define OTHER_SCENE_SCENE_SCENE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"

#include "scene/scene_tree.hpp"

#include "object/scene_object.hpp"

namespace other {

  class scene {
   public:
    scene() : tree(this) {}

    void register_object(scene_object* object, const std::string& name, const glm::vec3& world_position);
    void unregister_object(scene_object* object);

   private:
    friend class scene_tree;

    entt::registry registry;
    scene_tree tree;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_HPP