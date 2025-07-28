/**
 * @file math/definitions.hpp
 */
#ifndef OTHER_CORE_MATH_DEFINITIONS_HPP
#define OTHER_CORE_MATH_DEFINITIONS_HPP

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/formatting.hpp"
#include "serialization/reflection.hpp"

namespace other {

}  // namespace other

OTHER_REFLECT(
  glm::vec2,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable())
)

OTHER_REFLECT(
  glm::dvec2,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable())
)

OTHER_REFLECT(
  glm::uvec2,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable())
)

OTHER_REFLECT(
  glm::ivec2,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable())
)

OTHER_REFLECT(
  glm::vec3,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable())
)

OTHER_REFLECT(
  glm::dvec3,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable())
)
OTHER_REFLECT(
  glm::uvec3,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable())
)

OTHER_REFLECT(
  glm::ivec3,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable())
)

OTHER_REFLECT(
  glm::vec4,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable()),
  field(w, other::attr::serializable())
)

OTHER_REFLECT(
  glm::dvec4,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable()),
  field(w, other::attr::serializable())
)

OTHER_REFLECT(
  glm::uvec4,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable()),
  field(w, other::attr::serializable())
)

OTHER_REFLECT(
  glm::ivec4,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable()),
  field(w, other::attr::serializable())
)

OTHER_REFLECT(
  glm::quat,
  field(x, other::attr::serializable()),
  field(y, other::attr::serializable()),
  field(z, other::attr::serializable()),
  field(w, other::attr::serializable())
)

/// TODO: figure out how to reflect glm::mat2, mat3, and mat4

#endif  // OTHER_CORE_MATH_DEFINITIONS_HPP