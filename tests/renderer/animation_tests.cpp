/**
 * \file tests/renderer/animation_tests.cpp
 *
 * contract under test (doc 03 §2-4): assimp import emits a parents-first joint skeleton
 *  and immutable seconds-normalized clips, .oanim round-trips exactly through the
 *  field-codec primitives without ever aborting on malformed bytes, and the pure
 *  runtime primitives (pose / clip_binding / sample_clip / blend_poses / build_palette)
 *  behave headlessly over plain data.
 **/
#include <cmath>

#include <gtest/gtest.h>

#include <glm/gtc/matrix_transform.hpp>

#include "core/fnv.hpp"

#include "model/model_importer.hpp"
#include "model/pose.hpp"

#include "serialization/animation_serializer.hpp"

#include "other_test.hpp"

namespace other {

  class animation_tests : public other_test {
   protected:
    /// three-joint chain A -> B -> C with non-trivial bind TRS; inverse_bind matrices
    ///  are the exact inverses of the model-space bind chain, so the bind pose maps to
    ///  an identity palette by construction
    static skeleton make_chain_skeleton() {
      skeleton skel;
      skel.name = "chain";

      const auto add = [&skel](const char* name, int16_t parent, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale) {
        joint& j = skel.joints.emplace_back();
        j.name = name;
        j.name_hash = FNV(name);
        j.parent = parent;
        j.bind_position = pos;
        j.bind_rotation = rot;
        j.bind_scale = scale;
      };
      add("A", -1, { 0.f, 1.f, 0.f }, glm::angleAxis(glm::radians(30.f), glm::vec3(0.f, 1.f, 0.f)), { 1.f, 1.f, 1.f });
      add("B", 0, { 0.f, 2.f, 0.f }, glm::angleAxis(glm::radians(-45.f), glm::vec3(1.f, 0.f, 0.f)), { 2.f, 2.f, 2.f });
      add("C", 1, { 1.f, 0.5f, 0.f }, glm::quat(1.f, 0.f, 0.f, 0.f), { 1.f, 1.f, 1.f });

      glm::mat4 model_space[3];
      for (size_t i = 0; i < skel.joints.size(); ++i) {
        joint& j = skel.joints[i];
        const glm::mat4 local = glm::translate(glm::mat4(1.f), j.bind_position) * glm::mat4_cast(j.bind_rotation) * glm::scale(glm::mat4(1.f), j.bind_scale);
        model_space[i] = j.parent < 0 ? local : model_space[j.parent] * local;
        j.inverse_bind = glm::inverse(model_space[i]);
      }
      return skel;
    }

    static void expect_mat4_near(const glm::mat4& actual, const glm::mat4& expected, float tolerance = 0.0001f) {
      for (glm::length_t c = 0; c < 4; ++c) {
        for (glm::length_t r = 0; r < 4; ++r) {
          EXPECT_NEAR(actual[c][r], expected[c][r], tolerance) << "column " << c << " row " << r;
        }
      }
    }

    static animation_clip make_test_clip() {
      animation_clip clip;
      clip.name = "test_clip";
      clip.duration = 2.5f;

      joint_track& root = clip.joint_tracks.emplace_back();
      root.joint_name = "root";
      root.joint_name_hash = FNV("root");
      root.position_keyframes.push_back({ 0.f, glm::vec3(0.f, 0.f, 0.f) });
      root.position_keyframes.push_back({ 2.5f, glm::vec3(1.f, 2.f, 3.f) });
      root.rotation_keyframes.push_back({ 0.f, glm::quat(1.f, 0.f, 0.f, 0.f) });
      root.rotation_keyframes.push_back({ 1.25f, glm::quat(0.7071068f, 0.7071068f, 0.f, 0.f) });
      root.scale_keyframes.push_back({ 0.f, glm::vec3(1.f, 1.f, 1.f) });

      /// rotation-only second track anchors per-channel independence
      joint_track& arm = clip.joint_tracks.emplace_back();
      arm.joint_name = "arm";
      arm.joint_name_hash = FNV("arm");
      arm.rotation_keyframes.push_back({ 0.5f, glm::quat(0.f, 0.f, 1.f, 0.f) });
      return clip;
    }
  };

  TEST_F(animation_tests, oanim_round_trip) {
    const animation_clip clip = make_test_clip();
    const ostd::vector<uint8_t> bytes = serialization::serialize_animation_clip(clip);

    serialization::clip_parse_result result = serialization::parse_animation_clip(bytes);
    ASSERT_TRUE(result.success()) << result.error;

    const animation_clip& parsed = *result.clip;
    EXPECT_EQ(parsed.name, clip.name);
    EXPECT_FLOAT_EQ(parsed.duration, clip.duration);
    ASSERT_EQ(parsed.joint_tracks.size(), clip.joint_tracks.size());

    for (size_t t = 0; t < clip.joint_tracks.size(); ++t) {
      const joint_track& expected = clip.joint_tracks[t];
      const joint_track& actual = parsed.joint_tracks[t];
      EXPECT_EQ(actual.joint_name_hash, expected.joint_name_hash);
      EXPECT_EQ(actual.joint_name, expected.joint_name);

      ASSERT_EQ(actual.position_keyframes.size(), expected.position_keyframes.size());
      for (size_t k = 0; k < expected.position_keyframes.size(); ++k) {
        EXPECT_EQ(actual.position_keyframes[k].t, expected.position_keyframes[k].t);
        EXPECT_EQ(actual.position_keyframes[k].value, expected.position_keyframes[k].value);
      }
      ASSERT_EQ(actual.rotation_keyframes.size(), expected.rotation_keyframes.size());
      for (size_t k = 0; k < expected.rotation_keyframes.size(); ++k) {
        EXPECT_EQ(actual.rotation_keyframes[k].t, expected.rotation_keyframes[k].t);
        EXPECT_EQ(actual.rotation_keyframes[k].value, expected.rotation_keyframes[k].value);
      }
      ASSERT_EQ(actual.scale_keyframes.size(), expected.scale_keyframes.size());
      for (size_t k = 0; k < expected.scale_keyframes.size(); ++k) {
        EXPECT_EQ(actual.scale_keyframes[k].t, expected.scale_keyframes[k].t);
        EXPECT_EQ(actual.scale_keyframes[k].value, expected.scale_keyframes[k].value);
      }
    }

    /// serialize again: byte-identical output pins the format
    EXPECT_EQ(serialization::serialize_animation_clip(parsed), bytes);
  }

  TEST_F(animation_tests, oanim_rejects_malformed_bytes) {
    const ostd::vector<uint8_t> bytes = serialization::serialize_animation_clip(make_test_clip());

    /// wrong magic
    {
      ostd::vector<uint8_t> bad = bytes;
      bad[0] = 'X';
      serialization::clip_parse_result result = serialization::parse_animation_clip(bad);
      EXPECT_FALSE(result.success());
      EXPECT_NE(result.error.find("magic"), std::string::npos) << result.error;
    }

    /// unsupported format version
    {
      ostd::vector<uint8_t> bad = bytes;
      bad[4] = 0xFF;
      serialization::clip_parse_result result = serialization::parse_animation_clip(bad);
      EXPECT_FALSE(result.success());
      EXPECT_NE(result.error.find("format"), std::string::npos) << result.error;
    }

    /// truncation at every prefix length must error, never abort
    for (size_t len = 0; len < bytes.size(); ++len) {
      serialization::clip_parse_result result = serialization::parse_animation_clip(std::span<const uint8_t>{ bytes.data(), len });
      EXPECT_FALSE(result.success()) << "prefix of " << len << " bytes parsed as a whole clip";
    }

    /// trailing garbage is malformed, not silently ignored
    {
      ostd::vector<uint8_t> bad = bytes;
      bad.push_back(0xAB);
      serialization::clip_parse_result result = serialization::parse_animation_clip(bad);
      EXPECT_FALSE(result.success());
      EXPECT_NE(result.error.find("trailing"), std::string::npos) << result.error;
    }

    /// empty input
    EXPECT_FALSE(serialization::parse_animation_clip({}).success());
  }

  TEST_F(animation_tests, imported_skeleton_is_parents_first) {
    model_import_result result = import(filepath{ "tests/resources/models/bone-test-2-1.glb" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    const skeleton& skel = data.skel;
    ASSERT_FALSE(skel.empty());
    ASSERT_EQ(skel.joints.size(), 2u);

    for (size_t i = 0; i < skel.joints.size(); ++i) {
      const joint& j = skel.joints[i];
      /// parents ALWAYS precede children — build_palette (doc 03 §4) is a single forward pass
      EXPECT_LT(j.parent, static_cast<int16_t>(i));
      EXPECT_GE(j.parent, int16_t{ -1 });
      EXPECT_EQ(j.name_hash, FNV(j.name));
      EXPECT_FALSE(j.name.empty());
    }

    /// find_joint resolves every joint back to its own index
    for (size_t i = 0; i < skel.joints.size(); ++i) {
      EXPECT_EQ(skel.find_joint(skel.joints[i].name_hash), static_cast<int16_t>(i));
    }
    EXPECT_EQ(skel.find_joint(FNV("no_such_joint")), -1);

    /// the rigged submesh's vertex weights bind resolvable joints
    ASSERT_EQ(data.submeshes.size(), 1u);
    EXPECT_TRUE(data.submeshes[0].rigged);
    bool any_weighted = false;
    for (const vertex& v : data.vertices) {
      for (glm::length_t b = 0; b < 4; ++b) {
        if (v.bone_weights[b] > 0.f) {
          any_weighted = true;
          EXPECT_GE(v.bone_ids[b], 0);
          EXPECT_LT(v.bone_ids[b], static_cast<int32_t>(skel.joints.size()));
        }
      }
    }
    EXPECT_TRUE(any_weighted);
  }

  TEST_F(animation_tests, import_clip_extraction) {
    model_import_result result = import(filepath{ "tests/resources/models/bone-test-2-1.glb" });
    ASSERT_TRUE(result.data.has_value()) << result.error;
    const model_data& data = *result.data;

    ASSERT_EQ(data.clips.size(), 4u);
    for (const animation_clip& clip : data.clips) {
      EXPECT_FALSE(clip.name.empty());
      /// 40 ticks at 24 tps — seconds normalization is what distinguishes this from raw assimp data
      EXPECT_NEAR(clip.duration, 1.667f, 0.01f);
      ASSERT_FALSE(clip.joint_tracks.empty());

      for (const joint_track& track : clip.joint_tracks) {
        EXPECT_EQ(track.joint_name_hash, FNV(track.joint_name));
        /// every track binds a joint of this model's skeleton (channel-only joints are synthesized)
        EXPECT_NE(data.skel.find_joint(track.joint_name_hash), -1) << track.joint_name;

        /// keys are seconds-normalized and non-decreasing, inside the clip's duration
        const auto check_keys = [&](const auto& keys) {
          float last = 0.f;
          for (const auto& key : keys) {
            EXPECT_GE(key.t, last);
            EXPECT_LE(key.t, clip.duration + 0.001f);
            last = key.t;
          }
        };
        check_keys(track.position_keyframes);
        check_keys(track.rotation_keyframes);
        check_keys(track.scale_keyframes);
      }
    }

    /// the multi-track clip drives both joints
    const auto multi = std::ranges::find_if(data.clips, [](const animation_clip& c) { return c.joint_tracks.size() == 2; });
    ASSERT_NE(multi, data.clips.end());
  }

  TEST_F(animation_tests, sample_edges) {
    const skeleton skel = make_chain_skeleton();

    animation_clip clip;
    clip.name = "edges";
    clip.duration = 3.f;
    joint_track& track = clip.joint_tracks.emplace_back();
    track.joint_name = "A";
    track.joint_name_hash = FNV("A");
    track.position_keyframes.push_back({ 1.f, glm::vec3(1.f, 0.f, 0.f) });
    track.position_keyframes.push_back({ 2.f, glm::vec3(3.f, 0.f, 0.f) });
    track.position_keyframes.push_back({ 3.f, glm::vec3(3.f, 4.f, 0.f) });
    /// single-key channel is a constant at every sample time
    track.scale_keyframes.push_back({ 2.f, glm::vec3(5.f, 5.f, 5.f) });

    clip_binding binding;
    binding.build(clip, skel);

    pose out;
    const auto sample_at = [&](float time) {
      out.reset_to_bind(skel);
      sample_clip(clip, binding, time, out);
    };

    sample_at(0.5f);  /// before the first key clamps to it
    EXPECT_EQ(out.positions[0], glm::vec3(1.f, 0.f, 0.f));
    EXPECT_EQ(out.scales[0], glm::vec3(5.f, 5.f, 5.f));

    sample_at(2.f);  /// exact key
    EXPECT_EQ(out.positions[0], glm::vec3(3.f, 0.f, 0.f));

    sample_at(1.5f);  /// between keys lerps
    EXPECT_EQ(out.positions[0], glm::vec3(2.f, 0.f, 0.f));

    sample_at(2.5f);
    EXPECT_EQ(out.positions[0], glm::vec3(3.f, 2.f, 0.f));

    sample_at(10.f);  /// after the last key clamps to it
    EXPECT_EQ(out.positions[0], glm::vec3(3.f, 4.f, 0.f));
    EXPECT_EQ(out.scales[0], glm::vec3(5.f, 5.f, 5.f));

    /// channels without keys leave the pose untouched (bind rotation survives)
    EXPECT_EQ(out.rotations[0], skel.joints[0].bind_rotation);
  }

  TEST_F(animation_tests, sample_wraps_via_caller) {
    const skeleton skel = make_chain_skeleton();

    animation_clip clip;
    clip.name = "loop_me";
    clip.duration = 2.f;
    joint_track& track = clip.joint_tracks.emplace_back();
    track.joint_name = "A";
    track.joint_name_hash = FNV("A");
    track.position_keyframes.push_back({ 0.f, glm::vec3(0.f) });
    track.position_keyframes.push_back({ 2.f, glm::vec3(2.f, 0.f, 0.f) });

    clip_binding binding;
    binding.build(clip, skel);

    /// the math clamps past the end — no implicit wrapping
    pose out;
    out.reset_to_bind(skel);
    sample_clip(clip, binding, 3.f, out);
    EXPECT_EQ(out.positions[0], glm::vec3(2.f, 0.f, 0.f));

    /// loop policy lives with the caller (the component tick applies fmod before sampling)
    out.reset_to_bind(skel);
    sample_clip(clip, binding, std::fmod(3.f, clip.duration), out);
    EXPECT_EQ(out.positions[0], glm::vec3(1.f, 0.f, 0.f));
  }

  TEST_F(animation_tests, binding_partial) {
    const skeleton skel = make_chain_skeleton();

    /// tracks for A and C, plus a D this skeleton does not have — the retarget-lite anchor
    animation_clip clip;
    clip.name = "partial";
    clip.duration = 1.f;
    const auto add_track = [&clip](const char* joint_name, const glm::vec3& target) {
      joint_track& track = clip.joint_tracks.emplace_back();
      track.joint_name = joint_name;
      track.joint_name_hash = FNV(joint_name);
      track.position_keyframes.push_back({ 0.f, target });
    };
    add_track("A", glm::vec3(9.f, 0.f, 0.f));
    add_track("C", glm::vec3(0.f, 9.f, 0.f));
    add_track("D", glm::vec3(0.f, 0.f, 9.f));

    clip_binding binding;
    binding.build(clip, skel);
    ASSERT_EQ(binding.joint_of_track.size(), 3u);
    EXPECT_EQ(binding.joint_of_track[0], 0);
    EXPECT_EQ(binding.joint_of_track[1], 2);
    EXPECT_EQ(binding.joint_of_track[2], -1);

    pose out;
    out.reset_to_bind(skel);
    sample_clip(clip, binding, 0.f, out);

    EXPECT_EQ(out.positions[0], glm::vec3(9.f, 0.f, 0.f));
    /// unbound joint B keeps its bind transform
    EXPECT_EQ(out.positions[1], skel.joints[1].bind_position);
    EXPECT_EQ(out.positions[2], glm::vec3(0.f, 9.f, 0.f));
  }

  TEST_F(animation_tests, blend_halfway) {
    const skeleton skel = make_chain_skeleton();

    pose a;
    a.reset_to_bind(skel);
    a.positions[0] = glm::vec3(0.f);
    a.rotations[0] = glm::quat(1.f, 0.f, 0.f, 0.f);
    a.scales[0] = glm::vec3(1.f);

    pose b;
    b.reset_to_bind(skel);
    b.positions[0] = glm::vec3(2.f, 4.f, 6.f);
    b.rotations[0] = glm::angleAxis(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    b.scales[0] = glm::vec3(3.f);

    pose out;
    blend_poses(a, b, 0.5f, out);
    ASSERT_EQ(out.size(), skel.joints.size());

    EXPECT_EQ(out.positions[0], glm::vec3(1.f, 2.f, 3.f));
    EXPECT_EQ(out.scales[0], glm::vec3(2.f, 2.f, 2.f));

    /// halfway between identity and 90° about X is 45° about X, and it comes out normalized
    const glm::quat expected = glm::angleAxis(glm::radians(45.f), glm::vec3(1.f, 0.f, 0.f));
    EXPECT_NEAR(glm::length(out.rotations[0]), 1.f, 0.0001f);
    EXPECT_NEAR(std::abs(glm::dot(out.rotations[0], expected)), 1.f, 0.0001f);

    /// untouched joints blend between identical values and stay at bind
    EXPECT_EQ(out.positions[1], skel.joints[1].bind_position);
  }

  TEST_F(animation_tests, palette_identity) {
    const skeleton skel = make_chain_skeleton();

    pose bind_pose;
    bind_pose.reset_to_bind(skel);

    glm::mat4 palette[3] = { glm::mat4(0.f), glm::mat4(0.f), glm::mat4(0.f) };
    build_palette(skel, bind_pose, palette);

    /// bind pose ⇒ model-space chain × inverse_bind cancels for every joint — the
    ///  correctness anchor for ordering, parenting, and the palette convention
    for (size_t i = 0; i < skel.joints.size(); ++i) {
      expect_mat4_near(palette[i], glm::mat4(1.f));
    }
  }

}  // namespace other
