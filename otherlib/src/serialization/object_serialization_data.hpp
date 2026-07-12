/**
 * \file serialization/object_serialization_data.hpp
 **/
#ifndef OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_DATA_HPP
#define OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_DATA_HPP

#include <cstdint>
#include <vector>

#include "core/defines.hpp"

#include "object/scene_object.hpp"
#include "object/transform.hpp"

namespace other {
  namespace serialization {

#pragma pack(push, 1)
    struct parsed_scene_object {
    };

#pragma pack(pop)

  }  // namespace serialization
}  // namespace other

#endif  // OTHERLIB_SERIALIZATION_OBJECT_SERIALIZATION_DATA_HPP