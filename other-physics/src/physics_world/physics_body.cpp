/**
 * \file physics_world/physics_body.cpp
 **/
#include "physics_world/physics_body.hpp"

#include "math/matrix.hpp"

namespace other {

  glm::vec3 physics_body::get_previous_position() const {
    return get_position_from_transform(previous_transform);
  }

  glm::vec3 physics_body::get_current_position() const {
    return get_position_from_transform(current_transform);
  }

  glm::vec3 physics_body::get_interpolated_position() const {
    return get_position_from_transform(interpolated_transform);
  }

  glm::quat physics_body::get_previous_rotation() const {
    return get_rotation_from_transform(previous_transform);
  }

  glm::quat physics_body::get_current_rotation() const {
    return get_rotation_from_transform(current_transform);
  }

  glm::quat physics_body::get_interpolated_rotation() const {
    return get_rotation_from_transform(interpolated_transform);
  }

  glm::vec3 physics_body::get_position_from_transform(const glm::mat4& transform) const {
    return glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
  }

  glm::quat physics_body::get_rotation_from_transform(const glm::mat4& transform) const {
    glm::vec3 translation, scale;
    glm::quat rotation;
    decompose_mat4(transform, translation, rotation, scale);
    return rotation;
  }

}  // namespace other