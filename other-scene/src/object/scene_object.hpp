/**
 * @file object/scene_object.hpp
 */
#ifndef OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP
#define OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP

#include "core/defines.hpp"
#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

namespace other {

  class scene;

  struct scene_object {
    OTHER_REFLECTABLE(scene_object);
    natural_t id = 0;
    uint32_t registry_id = 0;

    std::string name = "SceneObject";
    bool visible = true;

    scene_object() = default;
  };

}  // namespace other

OTHER_REFLECT(
  other::scene_object,
  field(id, other::attr::serializable()),
  field(registry_id, other::attr::serializable()),
  field(name, other::attr::serializable()),
  field(visible, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP