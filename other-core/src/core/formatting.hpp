/**
 * \file core/formatting.hpp
 **/
#ifndef OTHER_CORE_FORMATTING_HPP
#define OTHER_CORE_FORMATTING_HPP

#include <iostream>

#include <glm/glm.hpp>

namespace other {

}  // namespace other

std::ostream& operator<<(std::ostream& os, const glm::vec2& vec);
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec);
std::ostream& operator<<(std::ostream& os, const glm::vec4& vec);

std::ostream& operator<<(std::ostream& os, const glm::ivec2& vec);
std::ostream& operator<<(std::ostream& os, const glm::ivec3& vec);
std::ostream& operator<<(std::ostream& os, const glm::ivec4& vec);

std::ostream& operator<<(std::ostream& os, const glm::mat2& mat);
std::ostream& operator<<(std::ostream& os, const glm::mat3& mat);
std::ostream& operator<<(std::ostream& os, const glm::mat4& mat);

#endif  // OTHER_CORE_FORMATTING_HPP