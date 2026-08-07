/**
 * \file model/pose.cpp
 **/
#include "model/pose.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/profiler.hpp"

namespace other {

  namespace {

    /// binary search + interpolate; clamps before the first and after the last key,
    ///  single-key tracks are constants. callers skip empty channels
    template <typename T, typename Mix>
    T sample_keys(const ostd::vector<keyframe<T>>& keys, float time, Mix&& mix) {
      if (keys.size() == 1 || time <= keys.front().t) {
        return keys.front().value;
      }
      if (time >= keys.back().t) {
        return keys.back().value;
      }

      /// first key strictly past @p time; the clamps above pin it inside [begin+1, end-1]
      const auto next = std::ranges::upper_bound(keys, time, {}, &keyframe<T>::t);
      const auto prev = next - 1;
      const float span = next->t - prev->t;
      const float alpha = span > 0.f ? (time - prev->t) / span : 0.f;
      return mix(prev->value, next->value, alpha);
    }

  }  // namespace

  void pose::reset_to_bind(const skeleton& skel) {
    PROFILE_SECTION("pose::reset_to_bind");
    const size_t joint_count = skel.joints.size();
    positions.resize(joint_count);
    rotations.resize(joint_count);
    scales.resize(joint_count);
    for (size_t i = 0; i < joint_count; ++i) {
      const joint& j = skel.joints[i];
      positions[i] = j.bind_position;
      rotations[i] = j.bind_rotation;
      scales[i] = j.bind_scale;
    }
  }

  void clip_binding::build(const animation_clip& clip, const skeleton& skel) {
    PROFILE_SECTION("clip_binding::build");
    joint_of_track.resize(clip.joint_tracks.size());
    for (size_t t = 0; t < clip.joint_tracks.size(); ++t) {
      joint_of_track[t] = skel.find_joint(clip.joint_tracks[t].joint_name_hash);
    }
  }

  void sample_clip(const animation_clip& clip, const clip_binding& binding, float time, pose& out) {
    OTHER_ASSERT(binding.joint_of_track.size() == clip.joint_tracks.size(),
                 "clip binding was built for a different clip ({} tracks bound, clip has {})", binding.joint_of_track.size(), clip.joint_tracks.size());
    PROFILE_SECTION("sample_clip");

    for (size_t t = 0; t < clip.joint_tracks.size(); ++t) {
      const int16_t joint_idx = binding.joint_of_track[t];
      if (joint_idx < 0) {
        continue;
      }
      OTHER_ASSERT(static_cast<size_t>(joint_idx) < out.size(), "bound joint {} out of range for a pose of {} joints", joint_idx, out.size());

      const joint_track& track = clip.joint_tracks[t];
      if (!track.position_keyframes.empty()) {
        out.positions[joint_idx] = sample_keys(track.position_keyframes, time,
                                               [](const glm::vec3& a, const glm::vec3& b, float alpha) { return glm::mix(a, b, alpha); });
      }
      if (!track.rotation_keyframes.empty()) {
        out.rotations[joint_idx] = sample_keys(track.rotation_keyframes, time,
                                               [](const glm::quat& a, const glm::quat& b, float alpha) { return glm::normalize(glm::slerp(a, b, alpha)); });
      }
      if (!track.scale_keyframes.empty()) {
        out.scales[joint_idx] = sample_keys(track.scale_keyframes, time,
                                            [](const glm::vec3& a, const glm::vec3& b, float alpha) { return glm::mix(a, b, alpha); });
      }
    }
  }

  void blend_poses(const pose& a, const pose& b, float alpha, pose& out) {
    OTHER_ASSERT(a.size() == b.size(), "blend_poses size mismatch ({} joints vs {})", a.size(), b.size());
    PROFILE_SECTION("blend_poses");

    const size_t joint_count = a.size();
    out.positions.resize(joint_count);
    out.rotations.resize(joint_count);
    out.scales.resize(joint_count);
    for (size_t i = 0; i < joint_count; ++i) {
      out.positions[i] = glm::mix(a.positions[i], b.positions[i], alpha);
      out.rotations[i] = glm::normalize(glm::slerp(a.rotations[i], b.rotations[i], alpha));
      out.scales[i] = glm::mix(a.scales[i], b.scales[i], alpha);
    }
  }

  void build_palette(const skeleton& skel, const pose& p, std::span<glm::mat4> out_palette) {
    const size_t joint_count = skel.joints.size();
    OTHER_ASSERT(p.size() == joint_count, "pose has {} joints, skeleton '{}' has {}", p.size(), skel.name, joint_count);
    OTHER_ASSERT(out_palette.size() >= joint_count, "palette span of {} cannot hold {} joints", out_palette.size(), joint_count);
    OTHER_ASSERT(joint_count <= kMaxBones, "skeleton '{}' exceeds the {}-joint cap", skel.name, kMaxBones);
    PROFILE_SECTION("build_palette");

    /// parents precede children (skeleton import invariant), so one forward pass
    ///  completes every model-space chain
    glm::mat4 model_space[kMaxBones];
    for (size_t i = 0; i < joint_count; ++i) {
      const joint& j = skel.joints[i];
      const glm::mat4 local = glm::translate(glm::mat4(1.f), p.positions[i]) *
        glm::mat4_cast(p.rotations[i]) *
        glm::scale(glm::mat4(1.f), p.scales[i]);
      model_space[i] = j.parent < 0 ? local : model_space[j.parent] * local;
      out_palette[i] = skel.root_transform * model_space[i] * j.inverse_bind;
    }
  }

}  // namespace other
