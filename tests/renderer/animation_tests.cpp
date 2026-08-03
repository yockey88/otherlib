/**
 * \file tests/renderer/animation_tests.cpp
 *
 * contract under test (doc 03 §2-3): assimp import emits a parents-first joint skeleton
 *  and immutable seconds-normalized clips, and .oanim round-trips exactly through the
 *  field-codec primitives without ever aborting on malformed bytes.
 **/
#include <gtest/gtest.h>

#include "core/fnv.hpp"

#include "model/model_importer.hpp"

#include "serialization/animation_serializer.hpp"

#include "other_test.hpp"

namespace other {

  class animation_tests : public other_test {
   protected:
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

}  // namespace other
