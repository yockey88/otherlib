/**
 * \file model/pose.hpp
 *
 * pure animation runtime primitives: poses, sampling, blending, palettes.
 * plain data in/out — nothing here knows scenes, assets, or graphs.
 **/
#ifndef OTHER_RENDERER_MODEL_POSE_HPP
#define OTHER_RENDERER_MODEL_POSE_HPP

#include <span>

#include "model/animation_clip.hpp"
#include "model/skeleton.hpp"

namespace other {

  /// local-space TRS per joint, index-aligned with skeleton::joints
  struct pose {
    ostd::vector<glm::vec3> positions;
    ostd::vector<glm::quat> rotations;
    ostd::vector<glm::vec3> scales;

    void reset_to_bind(const skeleton& skel);  /// sizes + fills from the joint bind TRS
    size_t size() const { return positions.size(); }
  };

  /// track index -> joint index, resolved once per (clip, skeleton) pair;
  ///  -1 = the clip track has no joint in this skeleton
  struct clip_binding {
    ostd::vector<int16_t> joint_of_track;
    void build(const animation_clip& clip, const skeleton& skel);
  };

  /// bound tracks overwrite their joint's TRS; unbound joints keep whatever @p out holds
  ///  (reset_to_bind first for a full-body sample). time clamps to key range; wrapping is caller's policy
  void sample_clip(const animation_clip& clip, const clip_binding& binding, float time, pose& out);

  /// component-wise lerp (positions/scales) + normalized slerp (rotations); a and b
  ///  must be the same size, @p out is resized (aliasing a or b is fine)
  void blend_poses(const pose& a, const pose& b, float alpha, pose& out);

  /// model-space walk (single forward pass — parents precede children), each palette
  ///  entry = root_transform * model_space * inverse_bind
  void build_palette(const skeleton& skel, const pose& p, std::span<glm::mat4> out_palette);

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_POSE_HPP
