/**
 * \file object/physics_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "physics_world/physics_body.hpp"

namespace other {

  struct physics_shape;

  struct physics_component {
    physics_body* body = nullptr;
    physics_shape* shape = nullptr;

    physics_body::settings settings;

    physics_component() = default;
    physics_component(const physics_body::settings& settings) : settings(settings) {}
  };

  struct physics_component_lua_proxy {
    physics_component* comp = nullptr;
  };

}  // namespace other

OTHER_REFLECT(
  other::physics_component,
  field(settings, other::attr::serializable("Physics Settings")))

#endif  // OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP