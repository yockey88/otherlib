/**
 * \file serialization/object_serialization.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_HPP
#define OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_HPP

#include <cstdint>

#include "core/defines.hpp"

#include "object/scene_object.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_object_to_bytes(const scene& s, const scene_object& obj);

  }  // namespace serialization
}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_HPP