/**
 * \file serialization/scene_serialization_data.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_DATA_HPP
#define OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_DATA_HPP

#include <cstdint>

#include "core/defines.hpp"

namespace other {
  namespace serialization {

    constexpr static natural_t kSceneNameOffset = sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint16_t);

#pragma pack(push, 1)
    struct parsed_scene {
      uint8_t marker_bytes = 0xFF;
      uint8_t scene_data_type = 0;  // 0 = external, 1 = inline
      uint8_t scene_index = 0;
      uint64_t scene_id = 0;
      uint16_t scene_name_length = 0;
      /// name
      uint16_t num_objects = 0;
      /// objects

      enum scene_data_type : uint8_t {
        EXTERNAL = 0,
        INLINE = 1
      };
    };
#pragma pack(pop)

  }  // namespace serialization
}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_DATA_HPP