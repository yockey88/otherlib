/**
 * \file object/light_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP

#include "serialization/reflection.hpp"

#include "renderer/gpu_structs.hpp"

#include "object/component.hpp"

namespace other {

  struct light_component : public component {
    std::vector<gpu::point_light> point_lights;
    std::vector<gpu::directional_light> directional_lights;

    light_component()
        : component(component::LIGHT) {}
  };

}  // namespace other

OTHER_REFLECT(
  other::light_component,
  field(point_lights, other::attr::serializable()),
  field(directional_lights, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_LIGHT_COMPONENT_HPP