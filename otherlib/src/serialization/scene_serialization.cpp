/**
 * \file serialization/scene_serialization.cpp
 **/
#include "serialization/scene_serialization.hpp"

#include <cstdint>
#include <sstream>

#include "serialization/object_serialization.hpp"
#include "serialization/scene_serialization_data.hpp"
#include "serialization/serialization.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_scene_to_bytes(const scene& s) {
      /**
       | 1 bytes | 1 byte | 1 byte | 8 bytes | 2 bytes  | <scene name> | 2 bytes    | <objects>    |
       |-------------------------------------------------------------------------------------------|
       |  marker | type   | index  |   id    | name len |   name       | obj count  |   objects    |
       **/
      std::vector<uint8_t> data = {
        0xFF,  /// marker for scene start
        parsed_scene::INLINE,
        0,  /// scene index
      };
      // write_value<uint64_t>(s.id, data);
      // write_value<uint16_t>((uint16_t)s.name.size(), data);
      // write_string_value(s.name, data);
      // write_value<uint16_t>((uint16_t)s.get_object_count(), data);

      // std::vector<uint64_t> object_ids = s.get_all_object_ids();
      // for (uint64_t id : object_ids) {
      //   if (id == 0) {
      //     /// root object, skip
      //     continue;
      //   }

      //   std::vector<uint8_t> object_data = write_object_to_bytes(s, s.get_object(id));
      //   data.append_range(object_data);
      // }

      return data;
    }

  }  // namespace serialization
}  // namespace other