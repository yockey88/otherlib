/**
 * \file serialization/scene_serialization.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_HPP
#define OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_HPP

#include "core/defines.hpp"

#include "scene/scene_graph.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_scene_to_bytes(const scene& s);

  }  // namespace serialization
}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_SCENE_SERIALIZATION_HPP