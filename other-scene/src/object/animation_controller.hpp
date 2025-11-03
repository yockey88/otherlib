/**
 * \file object/animation_controller.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP
#define OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP

#include <vector>

#include <glm/glm.hpp>

namespace other {

  struct animation;
  struct model;

  struct animation_controller {
    double animation_accumulator = 0.0;
    double animation_speed = 1.0;
    double animation_time = 0.0;

    uint32_t animation_index = 0;
    animation* anim_ptr;
    model* model_ptr;

    void update(double delta_time);

    double get_interpolation_factor(double current_time, double start_time, double end_time) const;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP