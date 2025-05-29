/**
 * \file math/orthonormal_basis.hpp
 **/
#ifndef OTHER_MATH_ORTHONORMAL_BASIS_HPP
#define OTHER_MATH_ORTHONORMAL_BASIS_HPP

#include <glm/glm.hpp>

namespace other {

  struct orthonormal_basis {
    glm::vec3 i, j, k;

    orthonormal_basis()
        : orthonormal_basis(glm::vec3(0, 0, 1)) {}
    orthonormal_basis(const glm::vec3& n);

    glm::vec3 to_local(const glm::vec3& v) const;
    glm::vec3 to_world(const glm::vec3& vec) const;
    glm::mat4 to_matrix() const;
  };

}  // namespace other

#endif  // OTHER_MATH_ORTHONORMAL_BASIS_HPP