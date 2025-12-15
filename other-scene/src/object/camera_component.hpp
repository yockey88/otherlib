/**
 * \file camera_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "renderer/camera.hpp"

namespace other {

  struct camera_component {
    camera camera;
  };

}  // namespace other

OTHER_REFLECT(
  other::camera_component,
  field(camera, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP