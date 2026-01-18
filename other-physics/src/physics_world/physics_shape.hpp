/**
 * \file physics_world/physics_shape.hpp
 **/
#ifndef OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP
#define OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP

#include "core/defines.hpp"

namespace other {

  struct physics_shape {
    integer_t id = 0;
    integer_t body_id = -1;
  };

}  // namespace other

#endif  // OTHER_PHYSICS_PHYSICS_WORLD_PHYSICS_SHAPE_HPP