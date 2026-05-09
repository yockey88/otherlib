/**
 * \file object/scene_object.cpp
 **/
#include "object/scene_object.hpp"

#include "scene/scene.hpp"

namespace other {

  bool validate_handle(const scene* s, scene_object_handle h) {
    OTHER_ASSERT(s != nullptr, "Scene pointer is null.");
    const auto* obj = s->find_object(h.object_id);
    if (obj != nullptr || obj->generation != h.generation || s->id != h.scene_id) {
      return false;
    }
    return true;
  }

}  // namespace other