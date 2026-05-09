/**
 * \file math/frustum.hpp
 **/
#ifndef OTHER_CORE_MATH_FRUSTUM_HPP
#define OTHER_CORE_MATH_FRUSTUM_HPP

#include <array>

#include <glm/glm.hpp>

#include "math/bounding_box.hpp"
#include "math/plane.hpp"

namespace other {

  struct frustum {
    enum plane_index {
      TOP_PLANE = 0,
      RIGHT_PLANE,
      BOTTOM_PLANE,
      LEFT_PLANE,
      NEAR_PLANE,
      FAR_PLANE,

      NUM_PLANES,
    };
    constexpr static size_t kPlaneCount = NUM_PLANES;
    std::array<plane, kPlaneCount> planes;

    void set_plane(size_t index, const plane& p);

    void extract_from_matrix(const glm::mat4& matrix);
    void set_from_camera(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up, float fov_y, float aspect_ratio, float near_plane, float far_plane);

    bool contains(const bounding_box& box) const;
    bool contains(const glm::vec3& point) const;
  };

}  // namespace other

#endif  // OTHER_CORE_MATH_FRUSTUM_HPP