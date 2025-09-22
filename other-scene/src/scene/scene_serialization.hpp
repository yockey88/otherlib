/**
 * \file scene/scene_serializer.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_SERIALIZATION_HPP
#define OTHER_SCENE_SCENE_SCENE_SERIALIZATION_HPP

#include "core/defines.hpp"

#include "scene/scene_graph.hpp"
#include "scene/scene_serialization_data.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_scene_to_bytes(const scene& s, uint8_t scene_index = 0);
    std::tuple<parsed_scene, std::string, natural_t> parse_single_scene(const std::span<const uint8_t> buffer);
    std::pair<std::vector<scene>, natural_t> parse_scene_list(const std::span<const uint8_t> buffer, natural_t num_scenes);

    std::string get_entity_tree_string(const std::span<const uint8_t> buffer);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_SERIALIZATION_HPP