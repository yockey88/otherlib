/**
 * \file math/coordinate_frame.cpp
 **/
#include "math/coordinate_frame.hpp"

namespace other {

  void coordinate_frame::shift_origin(const glm::vec3& new_origin) {
    active_origin = new_origin;
  }

  glm::vec3 coordinate_frame::to_local(const glm::vec3& world_position) const {
    return basis.to_local(world_position - active_origin);
  }

  glm::vec3 coordinate_frame::to_world(const glm::vec3& local_position) const {
    return basis.to_world(local_position) + active_origin;
  }

}  // namespace other