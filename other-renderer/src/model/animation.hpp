/**
 * \file model/animation.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_ANIMATION_HPP
#define OTHER_RENDERER_MODEL_ANIMATION_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

#include "glm/fwd.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace other {

  template <typename T>
  struct translation_key {
    uint32_t index;
    double time;
    T value;
    constexpr auto operator<=>(const translation_key&) const = default;
  };

  struct animation_channel {
    std::string node_name;
    glm::mat4 local_transform = glm::mat4(1.0f);
    std::vector<translation_key<glm::vec3>> position_keys;
    std::vector<translation_key<glm::quat>> rotation_keys;
    std::vector<translation_key<glm::vec3>> scale_keys;

    glm::mat4 get_transform_at_time(double time);
    glm::vec3 interpolated_position_at_time(double time) const;
    glm::quat interpolated_rotation_at_time(double time) const;
    glm::vec3 interpolated_scale_at_time(double time) const;

    size_t get_position_key_index_at_time(double time) const;
    size_t get_rotation_key_index_at_time(double time) const;
    size_t get_scale_key_index_at_time(double time) const;

    size_t get_interpolation_key_index(size_t index) const;

    const glm::vec3& get_position_at_time(double time) const;
    const glm::quat& get_rotation_at_time(double time) const;
    const glm::vec3& get_scale_at_time(double time) const;
  };

  struct animation {
    std::string name;

    double duration = 0.0f;
    double ticks_per_second = 25.0f;

    double current_time = 0.0f;

    std::vector<animation_channel> channels;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_ANIMATION_HPP