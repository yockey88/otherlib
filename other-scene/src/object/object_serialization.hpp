/**
 * \file object/object_serialization.hpp
 **/
#ifndef OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_HPP
#define OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_HPP

#include <cstdint>

#include "core/defines.hpp"

#include "object/object_serialization_data.hpp"
#include "object/scene_object.hpp"

namespace other {

  struct script_component;

  namespace serialization {

    ostd::vector<uint8_t> write_object_to_bytes(const scene& s, const scene_object& obj);
    std::pair<parsed_scene_object, natural_t> parse_single_object(const std::span<const uint8_t> buffer);
    std::pair<ostd::vector<parsed_scene_object>, natural_t> parse_object_list(const std::span<const uint8_t> buffer, natural_t num_objects);

    ostd::vector<uint8_t> write_attached_scripts_to_bytes(const script_component* obj);
    natural_t parse_attached_scripts_into_object(const std::span<const uint8_t> buffer, parsed_scene_object& object);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SCENE_OBJECT_OBJECT_SERIALIZATION_HPP