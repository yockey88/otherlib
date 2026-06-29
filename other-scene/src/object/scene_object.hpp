/**
 * @file object/scene_object.hpp
 */
#ifndef OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP
#define OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "renderer/render_stream_registry.hpp"

namespace other {

  class scene;

  struct scene_object {
    natural_t id = 0;
    uint32_t registry_id = 0;

    // incremented on reuse, helps detect stale references
    uint32_t generation = 0;
    std::string name = "SceneObject";
    bool visible = true;

    // each stream here must be configure to the other::vertex layout
    render_stream_registry geometry_streams;

    scene_object() = default;
  };

  struct scene_object_handle {
    natural_t scene_id = 0;
    natural_t object_id = 0;
    uint32_t generation = 0;
  };

  bool validate_handle(const scene* s, scene_object_handle h);

}  // namespace other

OTHER_REFLECT(
  other::scene_object,
  field(id, other::attr::serializable()),
  field(registry_id, other::attr::serializable()),
  field(name, other::attr::serializable()),
  field(visible, other::attr::serializable()))

#endif  // OTHER_SCENE_OBJECT_SCENE_OBJECT_HPP