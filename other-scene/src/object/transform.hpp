/**
 * \file object/transform.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_TRANSFORM_HPP
#define OTHER_SCENE_OBJECT_TRANSFORM_HPP

#include "math/definitions.hpp"
#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "object/component.hpp"

namespace other {

  struct transform : public component {
    orthonormal_basis local_basis = orthonormal_basis(glm::vec3(0, 1, 0));
    glm::vec3 local_position = { 0, 0, 0 };
    glm::vec3 local_scale = { 1, 1, 1 };
    glm::quat local_rotation_quat = glm::quat();

    void set_local_rotation(const glm::vec3& euler_angles) {
      local_rotation_quat = glm::quat(glm::radians(euler_angles));
    }

    glm::mat4 world_matrix(glm::mat4 parent_transform = glm::mat4(1.f)) const {
      return parent_transform * get_local_model_matrix();
    }

    glm::mat4 get_local_model_matrix() const {
      return local_translation_matrix() * local_rotation_matrix() * local_scale_matrix();
    }

    glm::mat4 local_translation_matrix() const { return glm::translate(glm::mat4(1.0f), local_position); }
    glm::mat4 local_rotation_matrix() const { return glm::toMat4(local_rotation_quat); }
    glm::mat4 local_scale_matrix() const { return glm::scale(glm::mat4(1.0f), local_scale); }

    transform()
        : component(component::TRANSFORM) {}
    transform(const glm::vec3& position)
        : component(component::TRANSFORM), local_position(position) {}
    transform(const glm::vec3& position, const glm::quat& rotation_quat)
        : component(component::TRANSFORM), local_position(position), local_rotation_quat(rotation_quat) {}
    transform(const glm::vec3& position, const glm::quat& rotation_quat, const glm::vec3& scale)
        : component(component::TRANSFORM), local_position(position), local_scale(scale), local_rotation_quat(rotation_quat) {}
    transform(const orthonormal_basis& basis, const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation_quat)
        : component(component::TRANSFORM), local_basis(basis), local_position(position), local_scale(scale), local_rotation_quat(rotation_quat) {}
  };

}  // namespace other

OTHER_REFLECT(
  other::transform,
  field(local_basis, other::attr::serializable()),
  field(local_position, other::attr::serializable("Position")),
  field(local_scale, other::attr::serializable("Scale")),
  field(local_rotation_quat, other::attr::serializable("Rotation"))
)

#endif  // OTHER_SCENE_OBJECT_TRANSFORM_HPP