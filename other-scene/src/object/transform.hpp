/**
 * \file object/transform.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_TRANSFORM_HPP
#define OTHER_SCENE_OBJECT_TRANSFORM_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "math/definitions.hpp"
#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

namespace other {

  struct transform {
    OTHER_REFLECTABLE(transform);

    orthonormal_basis local_basis = orthonormal_basis(glm::vec3(0, 1, 0));
    glm::vec3 world_position = { 0, 0, 0 };
    glm::vec3 world_scale = { 1, 1, 1 };

    glm::vec3 world_rotation = { 0, 0, 0 };
    glm::quat world_rotation_quat = glm::quat(1, 0, 0, 0);
  };

}  // namespace other

OTHER_REFLECT(
  other::transform,
  field(local_basis, other::attr::serializable()),
  field(world_position, other::attr::serializable()),
  field(world_scale, other::attr::serializable()),
  field(world_rotation, other::attr::serializable()),
  field(world_rotation_quat, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_TRANSFORM_HPP