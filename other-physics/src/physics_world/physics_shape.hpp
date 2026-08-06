/**
 * \file physics_world/physics_shape.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "math/bounding_box.hpp"
#include "serialization/reflection.hpp"

namespace other {

  /// stored in physics_shape_desc::shape_kind (uint32_t keeps the field scriptable through the ABI)
  enum physics_shape_kind : uint32_t {
    /// explicit "no collider": the body still simulates as a point mass
    PHYSICS_SHAPE_NONE = 0,
    PHYSICS_SHAPE_BOX = 1,
    PHYSICS_SHAPE_SPHERE = 2,
    PHYSICS_SHAPE_CAPSULE = 3,
    /// built from the entity's render model geometry
    PHYSICS_SHAPE_CONVEX_HULL = 4,
    /// statics only; dynamic bodies fall back to a convex hull with a warning
    PHYSICS_SHAPE_TRIANGLE_MESH = 5,

    NUM_PHYSICS_SHAPE_KINDS,
  };

  /// authored collider description; entity world scale is baked into the built shape, so a
  ///   scale edit rebuilds like any other desc edit
  struct physics_shape_desc {
    uint32_t shape_kind = PHYSICS_SHAPE_BOX;
    glm::vec3 half_extents = { 0.5f, 0.5f, 0.5f };  /// box
    float radius = 0.5f;                            /// sphere / capsule
    float half_height = 0.5f;                       /// capsule cylinder half-height (excludes the caps)
    /// derive box half extents from the render model's symmetric bounds instead of half_extents
    bool fit_render_bounds = false;

    bool operator==(const physics_shape_desc&) const = default;
  };

  inline bool shape_needs_geometry(const physics_shape_desc& desc) {
    return desc.shape_kind == PHYSICS_SHAPE_CONVEX_HULL || desc.shape_kind == PHYSICS_SHAPE_TRIANGLE_MESH;
  }

  struct physics_shape {
    integer_t id = 0;
    integer_t body_id = -1;

    /// what the backend last built, for the revalidation dirty-check; a fresh shape carries
    ///   the NUM_PHYSICS_SHAPE_KINDS sentinel so the first pass always builds
    physics_shape_desc applied;
    glm::vec3 applied_scale = { 1.f, 1.f, 1.f };
    bounding_box local_bounds = bounding_box::empty;

    bounding_box get_bounding_box() const;
  };

}  // namespace other

OTHER_REFLECT(
  other::physics_shape_desc,
  field(shape_kind, other::attr::serializable("Shape"),
        other::attr::clamp<uint32_t>(0, other::NUM_PHYSICS_SHAPE_KINDS - 1)),
  field(half_extents, other::attr::serializable("Half Extents")),
  field(radius, other::attr::serializable("Radius")),
  field(half_height, other::attr::serializable("Half Height")),
  field(fit_render_bounds, other::attr::serializable("Fit Render Bounds")))

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP
