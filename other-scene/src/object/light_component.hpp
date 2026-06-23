/**
 * \file object/light_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "renderer/gpu_structs.hpp"

#include "object/component.hpp"

namespace other {

  struct point_light {
    glm::vec3 position;
    glm::vec4 color;
  };

  struct direction_light {
    glm::vec3 direction;
    glm::vec4 color;
  };

  struct point_light_component : public component {
    point_light light;

    point_light_component()
        : component(component::POINT_LIGHT) {}
  };

  struct direction_light_component : public component {
    direction_light light;

    direction_light_component()
        : component(component::DIRECTION_LIGHT) {}
  };

}  // namespace other

OTHER_REFLECT(
  other::point_light_component,
  field(light, other::attr::serializable("Light")))

OTHER_REFLECT(
  other::direction_light_component,
  field(light, other::attr::serializable("Light")))

#endif  // OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP