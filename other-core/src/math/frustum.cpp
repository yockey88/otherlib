/**
 * \file math/frustum.cpp
 **/
#include "math/frustum.hpp"

namespace other {

  void frustum::set_plane(size_t index, const plane& p) {
    if (index >= kPlaneCount) {
      CORE_LOG_ERROR("Frustum plane index out of bounds: {}", index);
      return;
    }
    planes[index] = p;
  }

  void frustum::extract_from_matrix(const glm::mat4& matrix) {
  }

  void frustum::set_from_camera(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float fov_y, float aspect_ratio, float near_plane, float far_plane) {
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    float hh = tan(fov_y * 2.0f) * far_plane;
    float hw = hh * aspect_ratio;
    const glm::vec3 front_mult_far = forward * far_plane;

    set_plane(TOP_PLANE, plane{ position, glm::cross(right, glm::normalize(front_mult_far - up * hh)) });
    set_plane(BOTTOM_PLANE, plane{ position, glm::cross(glm::normalize(front_mult_far + up * hh), right) });
    set_plane(RIGHT_PLANE, plane{ position, glm::cross(glm::normalize(front_mult_far - right * hw), up) });
    set_plane(LEFT_PLANE, plane{ position, glm::cross(up, glm::normalize(front_mult_far + right * hw)) });
    set_plane(NEAR_PLANE, plane{ position + near_plane * forward, forward });
    set_plane(FAR_PLANE, plane{ position + front_mult_far, -forward });
  }

  bool frustum::contains(const bounding_box& box) const {
    bool contains_min = contains(box.min);
    bool contains_max = contains(box.max);
    return contains_min || contains_max;
  }

  bool frustum::contains(const glm::vec3& point) const {
    for (const plane& p : planes) {
      if (!p.is_facing_point(point)) {
        return false;
      }
    }
    return true;
  }

}  // namespace other