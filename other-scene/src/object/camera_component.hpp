/**
 * \file camera_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "renderer/camera.hpp"

#include "object/component.hpp"

namespace other {

  struct camera_component : public component {
    camera camera;
  };

  struct camera_component_lua_proxy {
    camera_component* native_pointer = nullptr;
  };

}  // namespace other

OTHER_REFLECT(
  other::camera_component,
  field(camera, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_CAMERA_COMPONENT_HPP