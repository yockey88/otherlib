/**
 * \file physics_world/physics_shape.cpp
 **/
#include "physics_world/physics_shape.hpp"

namespace other {

  bounding_box physics_shape::get_bounding_box() const {
    switch (applied.shape_kind) {
      case PHYSICS_SHAPE_BOX: {
        glm::vec3 he = applied.half_extents * applied_scale;
        return bounding_box(-he, he);
      }
      case PHYSICS_SHAPE_SPHERE: {
        float r = applied.radius * std::max({ std::abs(applied_scale.x), std::abs(applied_scale.y), std::abs(applied_scale.z) });
        return bounding_box(glm::vec3(-r), glm::vec3(r));
      }
      case PHYSICS_SHAPE_CAPSULE: {
        float r = applied.radius * std::max(std::abs(applied_scale.x), std::abs(applied_scale.z));
        float hh = applied.half_height * std::abs(applied_scale.y) + r;
        return bounding_box({ -r, -hh, -r }, { r, hh, r });
      }
      case PHYSICS_SHAPE_CONVEX_HULL:
      case PHYSICS_SHAPE_TRIANGLE_MESH:
        return local_bounds;  /// cached scaled-geometry bounds from the build
      case PHYSICS_SHAPE_NONE:
      default:
        return bounding_box::empty;  /// also the never-built sentinel
    }
  }

}  // namespace other
