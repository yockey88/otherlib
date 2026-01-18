/**
 * \file object/component_registry.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP
#define OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP

#include <vector>

#include "core/defines.hpp"

namespace other {

  struct component_registry {
    std::vector<natural_t> component_type_hashes;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP