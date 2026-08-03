/**
 * \file model/animation_clip.hpp
 */
#ifndef OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP
#define OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP

namespace other {

  template <typename T>
  struct keyframe {
    float t = 0.f;
    T value{};
  };

  struct joint_track {
    natural_t joint_name_hash = 0;
    ostd::vector<keyframe<glm::vec3>> position_keyframes;
    ostd::vector<keyframe<glm::quat>> rotation_keyframes;
    ostd::vector<keyframe<glm::vec3>> scale_keyframes;
  };

  struct animation_clip {
    std::string name;
    float duration = 0.f;
    ostd::vector<joint_track> joint_tracks;
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_ANIMATION_CLIP_HPP