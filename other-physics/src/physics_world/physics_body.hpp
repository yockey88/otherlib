/**
 * \file physics_world/physics_body.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "glm/fwd.hpp"

namespace other {

  enum physics_body_type {
    BODY_TYPE_STATIC,
    BODY_TYPE_DYNAMIC,
    BODY_TYPE_KINEMATIC
  };

  struct physics_body_settings {
    physics_body_type body_type = BODY_TYPE_STATIC;
    glm::mat4 world_transform = glm::mat4(1.0f);
    float mass = 1.0f;
  };

  struct physics_body {
    integer_t id = 0;
    natural_t scene_id = 0;

    integer_t shape_id = -1;

    bool active = false;

    physics_body_type body_type = BODY_TYPE_STATIC;
    float mass = 1.0f;

    glm::mat4 previous_transform = glm::mat4(1.0f);
    glm::mat4 current_transform = glm::mat4(1.0f);
    glm::mat4 interpolated_transform = glm::mat4(1.0f);

    glm::vec3 get_previous_position() const;
    glm::vec3 get_current_position() const;
    glm::vec3 get_interpolated_position() const;

    glm::quat get_previous_rotation() const;
    glm::quat get_current_rotation() const;
    glm::quat get_interpolated_rotation() const;

   private:
    glm::vec3 get_position_from_transform(const glm::mat4& transform) const;
    glm::quat get_rotation_from_transform(const glm::mat4& transform) const;
  };

}  // namespace other

OTHER_REFLECT(
  other::physics_body_settings,
  field(mass, other::attr::serializable())
)

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP