/**
 * \file object/script_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP

#include <cstdint>

namespace other {

  class scene_object;

  struct script_component {
    scene_object* object = nullptr;

    uint64_t script_object_id = 0;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_SCRIPT_COMPONENT_HPP
