/**
 * \file physics_world/physics_shape.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP

#include "core/defines.hpp"
#include "math/bounding_box.hpp"

namespace other {

  struct physics_shape {
    integer_t id = 0;
    integer_t body_id = -1;

    bounding_box get_bounding_box() const;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP