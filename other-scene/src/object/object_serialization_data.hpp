/**
 * \file object/object_serialization_data.hpp
 **/
#ifndef OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_DATA_HPP
#define OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_DATA_HPP

#include <cstdint>
#include <vector>

#include "core/defines.hpp"

#include "object/scene_object.hpp"
#include "object/transform.hpp"

namespace other {
  namespace serialization {

#pragma pack(push, 1)
    struct parsed_scene_object {
      scene_object object = {};
      transform obj_transform = {};

      natural_t parent_id = 0;
      std::vector<natural_t> children_ids = {};

      integer_t script_object_id = -1;

      struct attached_script {
        enum script_type : uint8_t {
          NONE = 0,
          DOTNET = 1,
          PYTHON = 2,
          LUA = 3
        };

        uint8_t type = NONE;
        std::string name = {};
        std::vector<uint8_t> data = {};
      };
      std::vector<attached_script> attached_scripts = {};
    };

#pragma pack(pop)

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_DATA_HPP