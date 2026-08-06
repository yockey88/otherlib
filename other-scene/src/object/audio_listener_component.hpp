/**
 * \file object/audio_listener_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_AUDIO_LISTENER_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_AUDIO_LISTENER_COMPONENT_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  /// where 3d audio is heard from. first active listener wins; without one the
  ///  reconciler falls back to the "main-camera" tagged object, then the origin
  struct audio_listener_component {
    bool active = true;

    /// runtime state, never reflected
    glm::vec3 prev_position{ 0.f };  /// finite-difference velocity; reseeded on restore
  };

}  // namespace other

OTHER_REFLECT(
  other::audio_listener_component,
  field(active, other::attr::serializable("Active")))

#endif  // OTHER_SCENE_OBJECT_AUDIO_LISTENER_COMPONENT_HPP
