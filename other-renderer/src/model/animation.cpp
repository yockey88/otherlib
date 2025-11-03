/**
 * \file model/animation.cpp
 **/
#include "model/animation.hpp"

namespace other {

  glm::mat4 animation_channel::get_transform_at_time(double time) {
    glm::vec3 position = interpolated_position_at_time(time);
    glm::quat rotation = interpolated_rotation_at_time(time);
    glm::vec3 scale = interpolated_scale_at_time(time);

    local_transform = glm::translate(glm::mat4(1.0f), position) *
      glm::mat4_cast(rotation) *
      glm::scale(glm::mat4(1.0f), scale);
    return local_transform;
  }

  namespace {

    float get_interpolation_factor(double current_time, double start_time, double end_time) {
      if (end_time - start_time == 0.0) {
        return 0.0f;
      }
      return static_cast<float>((current_time - start_time) / (end_time - start_time));
    }

  }  // namespace

  glm::vec3 animation_channel::interpolated_position_at_time(double time) const {
    size_t key_index = get_position_key_index_at_time(time);
    size_t next_key_index = (key_index + 1) % position_keys.size();

    double factor = get_interpolation_factor(time, position_keys[key_index].time, position_keys[next_key_index].time);
    return glm::mix(position_keys[key_index].value, position_keys[next_key_index].value, static_cast<float>(factor));
  }

  glm::quat animation_channel::interpolated_rotation_at_time(double time) const {
    size_t key_index = get_rotation_key_index_at_time(time);
    size_t next_key_index = (key_index + 1) % rotation_keys.size();

    double factor = get_interpolation_factor(time, rotation_keys[key_index].time, rotation_keys[next_key_index].time);
    return glm::slerp(rotation_keys[key_index].value, rotation_keys[next_key_index].value, static_cast<float>(factor));
  }

  glm::vec3 animation_channel::interpolated_scale_at_time(double time) const {
    size_t key_index = get_scale_key_index_at_time(time);
    size_t next_key_index = (key_index + 1) % scale_keys.size();

    double factor = get_interpolation_factor(time, scale_keys[key_index].time, scale_keys[next_key_index].time);
    return glm::mix(scale_keys[key_index].value, scale_keys[next_key_index].value, static_cast<float>(factor));
  }

  size_t animation_channel::get_position_key_index_at_time(double time) const {
    for (size_t i = 0; i < position_keys.size() - 1; ++i) {
      if (time < position_keys[i + 1].time) {
        return i;
      }
    }
    return position_keys.size() - 1;
  }

  size_t animation_channel::get_rotation_key_index_at_time(double time) const {
    for (size_t i = 0; i < rotation_keys.size() - 1; ++i) {
      if (time < rotation_keys[i + 1].time) {
        return i;
      }
    }
    return rotation_keys.size() - 1;
  }

  size_t animation_channel::get_scale_key_index_at_time(double time) const {
    for (size_t i = 0; i < scale_keys.size() - 1; ++i) {
      if (time < scale_keys[i + 1].time) {
        return i;
      }
    }
    return scale_keys.size() - 1;
  }

  size_t animation_channel::get_interpolation_key_index(size_t index) const {
    return (index + 1) % position_keys.size();
  }

  const glm::vec3& animation_channel::get_position_at_time(double time) const {
    size_t index = get_position_key_index_at_time(time);
    return position_keys[index].value;
  }

  const glm::quat& animation_channel::get_rotation_at_time(double time) const {
    size_t index = get_rotation_key_index_at_time(time);
    return rotation_keys[index].value;
  }

  const glm::vec3& animation_channel::get_scale_at_time(double time) const {
    size_t index = get_scale_key_index_at_time(time);
    return scale_keys[index].value;
  }

}  // namespace other