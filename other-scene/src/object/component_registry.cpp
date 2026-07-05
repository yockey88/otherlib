/**
 * \file object/component_registry.cpp
 **/
#include "object/component_registry.hpp"

namespace other {

  component_registration component_registry::get_registration(size_t type_hash) const {
    auto itr = registry.find(type_hash);
    OTHER_ASSERT(itr != registry.end(), "Component type with hash {} is not registered in the component registry.", type_hash);
    return itr->second;
  }

  bool component_registry::has_registration(natural_t type_id) const {
    return registry.find(type_id) != registry.end();
  }

}  // namespace other