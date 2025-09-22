/**
 * \file object/script_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP

#include <cstdint>

#include "serialization/reflection.hpp"

namespace other {

  struct scene_object;

  struct script_component {
    scene_object* object = nullptr;

    integer_t script_object_id = 0;
  };

}  // namespace other

OTHER_REFLECT(
  other::script_component,
  field(script_object_id, other::attr::serializable())
);

#endif  // OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
