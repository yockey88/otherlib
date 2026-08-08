/**
 * \file physics_world/physics_body.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "physics_world/physics_shape.hpp"

#include "glm/fwd.hpp"

namespace other {

  struct physics_body {
    /// values stored in settings::body_type (uint32_t keeps the field scriptable through the ABI)
    enum type : uint32_t {
      STATIC = 0,
      KINEMATIC = 1,
      DYNAMIC = 2,

      NUM_BODY_TYPES,
    };

    /// the complete AUTHORED body; pure data with a meaningful operator== so revalidation can
    ///  compare it against the built body. spawn pose is NOT here — passed to create_physics_body separately
    struct settings {
      uint32_t body_type = physics_body::STATIC;
      float mass = 1.0f;
      float friction = 0.5f;
      float restitution = 0.0f;
      float linear_damping = 0.05f;
      float angular_damping = 0.05f;
      /// per-body gravity multiplier; 0 = floats (spacesim ships)
      float gravity_factor = 1.0f;
      /// sensor body: overlaps report, nothing responds (dispatch lands with contacts)
      bool is_trigger = false;
      /// continuous collision detection for fast movers
      bool continuous_cd = false;
      physics_shape_desc shape;

      bool operator==(const settings&) const = default;
    };

    integer_t id = 0;
    uint64_t backend_id = 0;         /// the backend's own id for this body (jolt: BodyID bits)
    natural_t owner_object_id = 0;   /// scene-tree id of the owning scene object

    integer_t shape_id = -1;

    bool active = false;

    physics_body::type body_type = physics_body::STATIC;
    float mass = 1.0f;

    /// what this body was last built from, for the revalidation dirty-check
    settings applied_settings;

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
  other::physics_body::settings,
  field(body_type, other::attr::serializable("Body Type"),
        other::attr::clamp<uint32_t>(0, other::physics_body::NUM_BODY_TYPES - 1)),
  field(mass, other::attr::serializable("Mass")),
  field(friction, other::attr::serializable("Friction"), other::attr::clamp<float>(0.0f, 1.0f)),
  field(restitution, other::attr::serializable("Restitution"), other::attr::clamp<float>(0.0f, 1.0f)),
  field(linear_damping, other::attr::serializable("Linear Damping")),
  field(angular_damping, other::attr::serializable("Angular Damping")),
  field(gravity_factor, other::attr::serializable("Gravity Factor")),
  field(is_trigger, other::attr::serializable("Is Trigger")),
  field(continuous_cd, other::attr::serializable("Continuous CD")),
  field(shape, other::attr::serializable("Collider")))

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_BODY_HPP
