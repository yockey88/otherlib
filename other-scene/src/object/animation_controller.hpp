/**
 * \file object/animation_controller.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP
#define OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP

#include <vector>

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

#include "object/component.hpp"

namespace other {

  struct animation;
  struct model;

  struct animation_controller : public component {
    double animation_accumulator = 0.0;
    double animation_speed = 1.0;
    double animation_time = 0.0;

    uint32_t animation_index = 0;
    animation* anim_ptr = nullptr;
    model* model_ptr;
    glm::mat4 root_transform = glm::mat4(1.0f);

    animation_controller()
        : component(component::ANIMATION) {}

    void update(double delta_time);

    double get_interpolation_factor(double current_time, double start_time, double end_time) const;
  };

}  // namespace other

OTHER_REFLECT(
  other::animation_controller,
  field(animation_accumulator, other::attr::serializable()),
  field(animation_speed, other::attr::serializable()),
  field(animation_time, other::attr::serializable()),
  field(animation_index, other::attr::serializable())
  // field(root_transform)
)

#endif  // OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP