/**
 * \file object/animation_controller.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP
#define OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP

#include <vector>

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

#include "asset/asset.hpp"

namespace other {

  struct animation;
  struct model;

  struct animation_controller {
    double animation_accumulator = 0.0;
    double animation_speed = 1.0;
    double animation_time = 0.0;

    uint32_t animation_index = 0;
    // animation* anim_ptr = nullptr;
    // model* model_ptr;
    natural_t animation_asset_id = 0;
    natural_t model_asset_id = 0;
    glm::mat4 root_transform = glm::mat4(1.0f);

    void update(double delta_time);
    double get_interpolation_factor(double current_time, double start_time, double end_time) const;
  };

}  // namespace other

OTHER_REFLECT(
  other::animation_controller,
  field(animation_accumulator, other::attr::serializable("Animation Accumulator")),
  field(animation_speed, other::attr::serializable("Animation Speed")),
  field(animation_time, other::attr::serializable("Animation Time")),
  field(animation_index, other::attr::serializable("Animation Index")),
  field(animation_asset_id, other::attr::serializable("Animation"), other::attr::asset_identifier_field(other::asset::ANIMATION)))

#endif  // OTHER_SCENE_OBJECT_ANIMATION_CONTROLLER_HPP