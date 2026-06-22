/**
 * \file physics_world/physics_shape.cpp
 **/
#include "physics_world/physics_shape.hpp"

namespace other {

  bounding_box physics_shape::get_bounding_box() const {
    return bounding_box::empty;
  }

}  // namespace other