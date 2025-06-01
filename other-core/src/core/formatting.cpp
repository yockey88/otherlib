/**
 * \file core/formatting.hpp
 **/
#include "core/formatting.hpp"

namespace other {}  // namespace other

std::ostream& operator<<(std::ostream& os, const glm::vec2& vec) {
  return os << "vec2(" << vec.x << ", " << vec.y << ")";
}
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
  return os << "vec3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}
std::ostream& operator<<(std::ostream& os, const glm::vec4& vec) {
  return os << "vec4(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
}

std::ostream& operator<<(std::ostream& os, const glm::ivec2& vec) {
  return os << "ivec2(" << vec.x << ", " << vec.y << ")";
}
std::ostream& operator<<(std::ostream& os, const glm::ivec3& vec) {
  return os << "ivec3(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
}
std::ostream& operator<<(std::ostream& os, const glm::ivec4& vec) {
  return os << "ivec4(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")";
}

std::ostream& operator<<(std::ostream& os, const glm::mat2& mat) {
  return os << "mat2(\n";
  os << "  " << mat[0][0] << ", " << mat[0][1] << ",\n"
     << "  " << mat[1][0] << ", " << mat[1][1] << "\n";
  os << ")";
  return os;
}
std::ostream& operator<<(std::ostream& os, const glm::mat3& mat) {
  os << "mat3(\n";
  os << "  " << mat[0][0] << ", " << mat[0][1] << ", " << mat[0][2] << ",\n"
     << "  " << mat[1][0] << ", " << mat[1][1] << ", " << mat[1][2] << ",\n"
     << "  " << mat[2][0] << ", " << mat[2][1] << ", " << mat[2][2] << "\n";
  os << ")";
  return os;
}
std::ostream& operator<<(std::ostream& os, const glm::mat4& mat) {
  os << "mat4(\n";
  os << "  " << mat[0][0] << ", " << mat[0][1] << ", " << mat[0][2] << ", " << mat[0][3] << ",\n"
     << "  " << mat[1][0] << ", " << mat[1][1] << ", " << mat[1][2] << ", " << mat[1][3] << ",\n"
     << "  " << mat[2][0] << ", " << mat[2][1] << ", " << mat[2][2] << ", " << mat[2][3] << ",\n"
     << "  " << mat[3][0] << ", " << mat[3][1] << ", " << mat[3][2] << ", " << mat[3][3] << "\n";
  os << ")";
  return os;
}
