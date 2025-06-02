/**
 * @file object/scene_object.hpp
 */
#ifndef OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP
#define OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP

#include "core/defines.hpp"
#include "math/orthonormal_basis.hpp"

namespace other {

  struct scene_object {
    natural_t id = 0;
    uint32_t registry_id = 0;

    glm::vec3 world_position = { 0, 0, 0 };
    orthonormal_basis local_basis = orthonormal_basis(glm::vec3(0, 1, 0));

    std::string name = "SceneObject";
    bool visible = true;

    scene_object() = default;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP