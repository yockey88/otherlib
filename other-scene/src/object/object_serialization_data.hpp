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
      ostd::vector<natural_t> children_ids = {};

      integer_t script_object_id = -1;

      struct dotnet_object {
        std::string name = "";
        ostd::vector<uint8_t> dotnet_blob = {};
      } dotnet_obj;
      struct python_object {
      } python_obj;
      struct lua_object {
      } lua_obj;
    };

#pragma pack(pop)

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_DATA_HPP