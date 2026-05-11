/**
 * \file core/value_type.hpp
 **/
#ifndef OTHER_CORE_CORE_VALUE_TYPE_HPP
#define OTHER_CORE_CORE_VALUE_TYPE_HPP

#define GLM_ENABLE_EXPERIMENTAL
// #define GLM_FORCE_QUAT_DATA_WXYZ
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core/defines.hpp"

namespace other {

  static inline size_t get_value_type_size(value_type type) {
    switch (type) {
      case value_type::OEBOOL: return sizeof(bool);
      case value_type::CHAR: return sizeof(char);
      case value_type::STRING: return 0;  /// string size is dynamic
      case value_type::INT8: return sizeof(int8_t);
      case value_type::INT16: return sizeof(int16_t);
      case value_type::INT32: return sizeof(int32_t);
      case value_type::INT64: return sizeof(int64_t);
      case value_type::UINT8: return sizeof(uint8_t);
      case value_type::UINT16: return sizeof(uint16_t);
      case value_type::UINT32: return sizeof(uint32_t);
      case value_type::UINT64: return sizeof(uint64_t);
      case value_type::FLOAT: return sizeof(float);
      case value_type::DOUBLE: return sizeof(double);
      case value_type::VEC2: return sizeof(glm::vec2);
      case value_type::VEC3: return sizeof(glm::vec3);
      case value_type::VEC4: return sizeof(glm::vec4);
      case value_type::IVEC2: return sizeof(glm::ivec2);
      case value_type::IVEC3: return sizeof(glm::ivec3);
      case value_type::IVEC4: return sizeof(glm::ivec4);
      case value_type::MAT2: return sizeof(glm::mat2);
      case value_type::MAT3: return sizeof(glm::mat3);
      case value_type::MAT4: return sizeof(glm::mat4);
      case value_type::QUATERNION: return sizeof(glm::quat);
      case value_type::OPAQUE_HANDLE: return sizeof(void*);
      case value_type::BYTE_BUFFER: return 0;
      default: return sizeof(void*);
    }
  }

}  // namespace other

#endif  // OTHER_CORE_CORE_VALUE_TYPE_HPP