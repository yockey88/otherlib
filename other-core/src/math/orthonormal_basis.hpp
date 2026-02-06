/**
 * @file math/orthonormal_basis.hpp
 */
#ifndef OTHER_CORE_MATH_ORTHONORMAL_BASIS_HPP
#define OTHER_CORE_MATH_ORTHONORMAL_BASIS_HPP

#include <glm/glm.hpp>

#include "math/definitions.hpp"
#include "serialization/reflection.hpp"

namespace other {

  struct orthonormal_basis {
    glm::vec3 i, j, k;

    orthonormal_basis()
        : orthonormal_basis(glm::vec3(0, 1, 0)) {}
    orthonormal_basis(const glm::vec3& n)
        : orthonormal_basis(find_first_non_zero(glm::normalize(n)), n) {}
    orthonormal_basis(const glm::vec3& reference_vector, const glm::vec3& n);
    static orthonormal_basis from_rotation(const glm::quat& rotation);
    static orthonormal_basis from_rotation(const glm::vec3& euler_angles);
    static orthonormal_basis from_matrix(const glm::mat4& matrix);
    static orthonormal_basis from_matrix(const glm::mat3& matrix);

    glm::vec3 to_local(const glm::vec3& v) const;
    glm::vec3 to_world(const glm::vec3& vec) const;
    glm::mat4 to_matrix() const;

   private:
    glm::vec3 find_first_non_zero(const glm::vec3& v) const;

    void check_negative_zero();
  };

}  // namespace other

OTHER_REFLECT(
  other::orthonormal_basis,
  field(i, other::attr::serializable()),
  field(j, other::attr::serializable()),
  field(k, other::attr::serializable())
)

#endif  // OTHER_CORE_MATH_ORTHONORMAL_BASIS_HPP