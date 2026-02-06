/**
 * \file object/physics_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "physics_world/physics_body.hpp"

#include "object/component.hpp"

namespace other {

  struct physics_shape;

  struct physics_component : public component {
    physics_body* body = nullptr;
    physics_shape* shape = nullptr;

    physics_body_settings settings;

    physics_component()
        : component(component::PHYSICS) {}
    physics_component(const physics_body_settings& settings)
        : component(component::PHYSICS), settings(settings) {}
  };

}  // namespace other

OTHER_REFLECT(
  other::physics_component,
  field(settings, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_PHYSICS_COMPONENT_HPP