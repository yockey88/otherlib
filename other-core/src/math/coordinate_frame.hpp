/**
 * \file math/coordinate_frame.hpp
 **/
#ifndef OTHER_CORE_MATH_COORDINATE_FRAME_HPP
#define OTHER_CORE_MATH_COORDINATE_FRAME_HPP

#include "math/orthonormal_basis.hpp"

namespace other {

  class coordinate_frame {
   public:
    coordinate_frame()
        : basis(glm::vec3(0, 1, 0)) {}
    coordinate_frame(const orthonormal_basis& basis)
        : basis(basis) {}

    void shift_origin(const glm::vec3& new_origin);

    inline glm::mat4 transform() const {
      return glm::translate(glm::mat4(1.0f), active_origin) * basis.to_matrix();
    }
    glm::vec3 to_local(const glm::vec3& world_position) const;
    glm::vec3 to_world(const glm::vec3& local_position) const;

   private:
    glm::vec3 active_origin = glm::vec3(0.0f);

    orthonormal_basis basis;
  };

}  // namespace other

#endif  // !OTHER_CORE_MATH_COORDINATE_FRAME_HPP