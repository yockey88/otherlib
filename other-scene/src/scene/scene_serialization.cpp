/**
 * \file scene/scene_serialization.cpp
 **/
#include "scene/scene_serialization.hpp"

#include <cstdint>
#include <sstream>

#include "serialization/serialization.hpp"

#include "object/object_serialization.hpp"
#include "object/object_serialization_data.hpp"
#include "scene/scene_serialization_data.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_scene_to_bytes(const scene& s, uint8_t scene_index) {
      /**
       | 1 bytes | 1 byte | 1 byte | 8 bytes | 2 bytes  | <scene name> | 2 bytes    | <objects>    |
       |-------------------------------------------------------------------------------------------|
       |  marker | type   | index  |   id    | name len |   name       | obj count  |   objects    |
       **/
      std::vector<uint8_t> data = {
        0xFF,  /// marker for scene start
        parsed_scene::INLINE,
        scene_index,  /// scene index
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

    std::tuple<parsed_scene, std::string, natural_t> parse_single_scene(const std::span<const uint8_t> buffer) {
      OTHER_ASSERT(sizeof(parsed_scene) < buffer.size(), "Buffer too small to contain scene description");

      size_t cursor = 0;
      parsed_scene scene_desc = {};  // peek_at_value<parsed_scene>(buffer, cursor);
      // cursor += kSceneNameOffset;
      // OTHER_ASSERT(cursor + scene_desc.scene_name_length <= buffer.size(), "Buffer too small to contain scene name");

      std::string name = "";  // read_string_value(buffer, scene_desc.scene_name_length, cursor);
      // scene_desc.num_objects = read_value<uint16_t>(buffer, cursor);

      return { scene_desc, name, cursor };
    }

    std::pair<scene, natural_t> parse_scene(const std::span<const uint8_t> buffer) {
      size_t cur = 0;
      // auto [scene_desc, scene_name, bytes_read] = parse_single_scene(buffer);
      cur += 0;  // bytes_read;

      scene scene1;
      // scene1.name = scene_name;
      // scene1.id = scene_desc.scene_id;

      // auto [objects, obj_bytes_read] = parse_object_list(buffer.subspan(cur), scene_desc.num_objects);
      // cur += obj_bytes_read;

      // scene1.add_objects(objects);

      return { std::move(scene1), cur };
    }

    std::pair<std::vector<scene>, natural_t> parse_scene_list(const std::span<const uint8_t> buffer, natural_t num_scenes) {
      std::vector<scene> scenes = {};
      size_t cur = 0;

      // for (uint16_t i = 0; i < num_scenes && cur < buffer.size(); ++i) {
      //   auto [scene1, bytes_read] = parse_scene(buffer.subspan(cur));
      //   cur += bytes_read;
      //   scenes.push_back(std::move(scene1));
      // }

      return { std::move(scenes), cur };
    }

    namespace detail {

      std::string get_entity_and_children_string(const parsed_scene_object& obj, const std::vector<parsed_scene_object>& all_objects, size_t indent_level = 0) {
        std::stringstream ss;
        // std::string indent(indent_level * 2, ' ');
        // ss << indent << " '" << obj.object.name << "' (ID: " << obj.object.id << ")\n";

        // for (uint64_t child_id : obj.children_ids) {
        //   auto it = std::find_if(all_objects.begin(), all_objects.end(), [&](const parsed_scene_object& o) { return o.object.id == child_id; });
        //   if (it != all_objects.end()) {
        //     ss << get_entity_and_children_string(*it, all_objects, indent_level + 1);
        //   }
        // }

        return ss.str();
      }

    }  // namespace detail

    std::string get_entity_tree_string(const std::span<const uint8_t> buffer) {
      std::stringstream ss;
      // auto [s, sname, bytes_read] = parse_single_scene(buffer);
      // ss << "Scene '" << sname << "' (ID: " << s.scene_id << ")\n";

      // auto [objects, obj_bytes_read] = parse_object_list(buffer.subspan(bytes_read), s.num_objects);
      // ss << "Objects (" << objects.size() << "):\n";
      // for (const auto& obj : objects) {
      //   if (obj.parent_id == 0) {
      //     ss << detail::get_entity_and_children_string(obj, objects, 1);
      //   }
      // }
      return ss.str();
    }

  }  // namespace serialization
}  // namespace other