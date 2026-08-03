/**
 * \file model/animation_clip.hpp
 */
#ifndef OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP
#define OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/defines.hpp"

namespace other {

  template <typename T>
  struct keyframe {
    float t = 0.f;  /// seconds; ticks_per_second dies at import
    T value{};
  };

  struct joint_track {
    natural_t joint_name_hash = 0;  /// binds to a joint by FNV(name), not index — survives retarget-lite
    std::string joint_name;         /// tooling/debug only; runtime binds by the hash
    ostd::vector<keyframe<glm::vec3>> position_keyframes;
    ostd::vector<keyframe<glm::quat>> rotation_keyframes;
    ostd::vector<keyframe<glm::vec3>> scale_keyframes;
  };

  /// IMMUTABLE after load — no playback state, const everywhere. playback time,
  ///  looping, and speed live with the player (doc 03 §5)
  struct animation_clip {
    std::string name;
    float duration = 0.f;  /// seconds
    ostd::vector<joint_track> joint_tracks;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP
