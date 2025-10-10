/**
 * \file project/project_serialization.hpp
 **/
#ifndef OTHER_OTHERLIB_PROJECT_PROJECT_SERIALIZATION_HPP
#define OTHER_OTHERLIB_PROJECT_PROJECT_SERIALIZATION_HPP

#include <cstdint>
#include <span>

#include "core/defines.hpp"

namespace other {

  // #pragma pack(push, 1)
  //   struct project_description {
  //     uint16_t magic_header = 0;
  //     uint8_t version[3] = {};
  //     uint16_t num_scenes = 0;
  //   };
  // #pragma pack(pop)

  //   constexpr static uint16_t kProjectFileMagicHeader = 0x594F;  // 'OY'
  //   constexpr static natural_t kSceneListOffset = sizeof(project_description);

  //   static inline project_description parse_project_description(const std::span<const uint8_t> buffer) {
  //     project_description proj = *reinterpret_cast<const project_description*>(buffer.data());
  //     return proj;
  //   }

}  // namespace other

#endif  // OTHER_OTHERLIB_PROJECT_PROJECT_SERIALIZATION_HPP