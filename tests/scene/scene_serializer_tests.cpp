/**
 * \file tests/scene/scene_serializer_tests.cpp
 *
 * live scene <-> document conversions and the snapshot store/restore primitive that
 * play/stop restore and editor undo/redo ride on.
 **/
#include <gtest/gtest.h>

#include "other_test.hpp"

#include "object/animation_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/grid_component.hpp"
#include "object/light_component.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene.hpp"
#include "serialization/scene_serializer.hpp"

namespace other {

  class scene_serializer_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }
  };

  using namespace serialization;

  namespace {

    /// a small scene with hierarchy, tags, and a spread of component types
    void populate_test_scene(scene& s) {
      scene_object& parent = s.create_object("Parent");
      s.set_transform(parent.id, transform(glm::vec3(1.f, 2.f, 3.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(2.f, 2.f, 2.f)));
      s.add_object_tag(parent.id, "sun");

      grid_component grid = {};
      grid.coordinate_system = GRID_COORDINATES_POLAR;
      grid.extent = 33;
      grid.sector_count = 9;
      s.add_component<grid_component>(&parent, std::move(grid));

      scene_object& child = s.create_object("Child", &parent);
      s.set_transform(child.id, transform(glm::vec3(0.f, -1.f, 0.f)));
      child.visible = false;

      point_light_component light = {};
      light.light.position = glm::vec3(2.f, 4.f, 2.f);
      light.light.color = glm::vec4(1.f, 0.44f, 0.77f, 1.f);
      s.add_component<point_light_component>(&child, std::move(light));

      s.get_storage().clear_color = glm::vec4(0.5f, 0.25f, 0.125f, 1.f);
    }

    void expect_test_scene_state(scene& s) {
      scene_object* parent = s.find_object(std::string_view{ "Parent" });
      ASSERT_NE(parent, nullptr);
      scene_object* child = s.find_object(std::string_view{ "Child" });
      ASSERT_NE(child, nullptr);

      /// hierarchy
      const scene_object* childs_parent = s.get_parent(child->id);
      ASSERT_NE(childs_parent, nullptr);
      EXPECT_EQ(childs_parent->id, parent->id);

      /// object fields + tags
      EXPECT_FALSE(child->visible);
      EXPECT_TRUE(s.object_has_tag(parent->id, "sun"));

      /// transforms
      const transform& parent_transform = s.get_transform(parent->id);
      EXPECT_EQ(parent_transform.local_position, glm::vec3(1.f, 2.f, 3.f));
      EXPECT_EQ(parent_transform.local_scale, glm::vec3(2.f, 2.f, 2.f));
      const transform& child_transform = s.get_transform(child->id);
      EXPECT_EQ(child_transform.local_position, glm::vec3(0.f, -1.f, 0.f));

      /// components
      grid_component* grid = s.try_get_component<grid_component>(parent->id);
      ASSERT_NE(grid, nullptr);
      EXPECT_EQ(grid->coordinate_system, (uint32_t)GRID_COORDINATES_POLAR);
      EXPECT_EQ(grid->extent, 33u);
      EXPECT_EQ(grid->sector_count, 9u);

      point_light_component* light = s.try_get_component<point_light_component>(child->id);
      ASSERT_NE(light, nullptr);
      EXPECT_EQ(light->light.position, glm::vec3(2.f, 4.f, 2.f));
      EXPECT_EQ(light->light.color, glm::vec4(1.f, 0.44f, 0.77f, 1.f));

      /// scene-level state
      EXPECT_EQ(s.get_storage().clear_color, glm::vec4(0.5f, 0.25f, 0.125f, 1.f));
    }

  }  // namespace

  TEST_F(scene_serializer_tests, capture_then_instantiate_reproduces_the_scene) {
    /// managed script objects are keyed by object name environment-wide, so the source
    ///  scene must be gone before another scene instantiates the same names
    scene_document doc = {};
    {
      scene source("Source Scene");
      populate_test_scene(source);
      doc = capture_scene(source, default_codec_services());
    }

    EXPECT_EQ(doc.objects.size(), 2u);
    /// parents precede children in the document
    EXPECT_EQ(doc.objects[0].name, "Parent");
    EXPECT_EQ(doc.objects[1].name, "Child");
    EXPECT_EQ(doc.objects[1].parent_file_id, doc.objects[0].file_id);

    scene target("Target Scene");
    instantiate_scene(target, doc, default_codec_services());
    expect_test_scene_state(target);
  }

  TEST_F(scene_serializer_tests, capture_is_stable_across_a_roundtrip) {
    scene_document doc = {};
    {
      scene source("Source Scene");
      populate_test_scene(source);
      doc = capture_scene(source, default_codec_services());
    }

    scene target("Target Scene");
    instantiate_scene(target, doc, default_codec_services());

    /// a second capture from the instantiated scene must byte-match the first document's
    ///  payloads (ids may differ, so compare component payloads + structure)
    const scene_document doc2 = capture_scene(target, default_codec_services());
    ASSERT_EQ(doc.objects.size(), doc2.objects.size());
    for (size_t i = 0; i < doc.objects.size(); ++i) {
      EXPECT_EQ(doc.objects[i].name, doc2.objects[i].name);
      EXPECT_EQ(doc.objects[i].visible, doc2.objects[i].visible);
      EXPECT_EQ(doc.objects[i].tags, doc2.objects[i].tags);
      ASSERT_EQ(doc.objects[i].components.size(), doc2.objects[i].components.size()) << doc.objects[i].name;
      for (size_t c = 0; c < doc.objects[i].components.size(); ++c) {
        EXPECT_EQ(doc.objects[i].components[c].key_hash, doc2.objects[i].components[c].key_hash);
        EXPECT_EQ(doc.objects[i].components[c].payload, doc2.objects[i].components[c].payload) << doc.objects[i].name;
      }
    }
  }

  TEST_F(scene_serializer_tests, snapshot_restore_undoes_mutations) {
    scene s("Snapshot Scene");
    populate_test_scene(s);

    const ostd::vector<uint8_t> snapshot = s.capture_snapshot();
    EXPECT_FALSE(snapshot.empty());

    /// mutate everything restorable: move, retag, delete, create
    scene_object* parent = s.find_object(std::string_view{ "Parent" });
    ASSERT_NE(parent, nullptr);
    s.set_transform(parent->id, transform(glm::vec3(9.f, 9.f, 9.f)));
    s.remove_object_tag(parent->id, "sun");
    scene_object* child = s.find_object(std::string_view{ "Child" });
    ASSERT_NE(child, nullptr);
    s.destroy_object(child->id);
    scene_object& imposter = s.create_object("Imposter");
    (void)imposter;
    s.get_storage().clear_color = glm::vec4(0.f);

    s.restore_snapshot(snapshot);

    expect_test_scene_state(s);
    EXPECT_EQ(s.find_object(std::string_view{ "Imposter" }), nullptr);
    /// root + Parent + Child
    EXPECT_EQ(s.get_object_count(), 3u);
  }

  TEST_F(scene_serializer_tests, play_stop_restores_pre_play_state) {
    scene s("Play Scene");
    populate_test_scene(s);

    /// an id held across the play/stop cycle, editor-selection style
    scene_object* pre_play_parent = s.find_object(std::string_view{ "Parent" });
    ASSERT_NE(pre_play_parent, nullptr);
    const natural_t pre_play_parent_id = pre_play_parent->id;
    script_component* pre_play_script = s.try_get_component<script_component>(pre_play_parent_id);
    ASSERT_NE(pre_play_script, nullptr);
    const integer_t pre_play_script_id = pre_play_script->script_object_id;

    s.play();
    EXPECT_TRUE(s.is_playing());

    /// gameplay-style mutations while playing
    scene_object* parent = s.find_object(std::string_view{ "Parent" });
    ASSERT_NE(parent, nullptr);
    s.set_transform(parent->id, transform(glm::vec3(-5.f, 0.f, 12.f)));
    scene_object* child = s.find_object(std::string_view{ "Child" });
    ASSERT_NE(child, nullptr);
    s.destroy_object(child->id);

    s.stop();
    EXPECT_FALSE(s.is_playing());

    expect_test_scene_state(s);

    /// stop's restore reassigns runtime ids: a pre-play id must resolve to null (not a
    ///  live object, not UB), and the object's name is its stable identity across the
    ///  restore — the contract editor selection re-resolution depends on
    EXPECT_EQ(s.find_object(pre_play_parent_id), nullptr);
    scene_object* restored_parent = s.find_object(std::string_view{ "Parent" });
    ASSERT_NE(restored_parent, nullptr);
    EXPECT_NE(restored_parent->id, pre_play_parent_id);

    /// stop is a disable, not a remove: the script object (and its managed instance)
    ///  survives the restore and is rebound to the re-created scene object, so
    ///  Awake/Remove stay reserved for load/unload/reload
    script_component* restored_script = s.try_get_component<script_component>(restored_parent->id);
    ASSERT_NE(restored_script, nullptr);
    EXPECT_EQ(restored_script->script_object_id, pre_play_script_id);
  }

  TEST_F(scene_serializer_tests, animation_component_play_serialize) {
    scene s("Anim Scene");
    scene_object& dancer = s.create_object("Dancer");

    animation_component anim = {};
    anim.clip_name = "walk";
    anim.playing = true;
    anim.looping = false;
    anim.speed = 2.f;
    anim.time = 0.75f;
    s.add_component<animation_component>(&dancer, std::move(anim));

    const ostd::vector<uint8_t> snapshot = s.capture_snapshot();

    /// gameplay-style mutations while "playing"
    animation_component* live = s.try_get_component<animation_component>(dancer.id);
    ASSERT_NE(live, nullptr);
    live->clip_name = "idle";
    live->playing = false;
    live->looping = true;
    live->speed = 1.f;
    live->time = 0.f;
    /// fake resolved runtime state; a restore must never resurrect it
    live->working_pose.positions.push_back(glm::vec3(1.f));

    s.restore_snapshot(snapshot);

    scene_object* restored = s.find_object(std::string_view{ "Dancer" });
    ASSERT_NE(restored, nullptr);
    animation_component* comp = s.try_get_component<animation_component>(restored->id);
    ASSERT_NE(comp, nullptr);

    EXPECT_EQ(comp->clip_name, "walk");
    EXPECT_TRUE(comp->playing);
    EXPECT_FALSE(comp->looping);
    EXPECT_FLOAT_EQ(comp->speed, 2.f);
    /// mid-clip time round-trips — play/stop resumes the pre-play pose
    EXPECT_FLOAT_EQ(comp->time, 0.75f);

    /// runtime state never serializes; the next tick rebuilds it from the resolved clip
    EXPECT_EQ(comp->clip, nullptr);
    EXPECT_EQ(comp->bound_skeleton, nullptr);
    EXPECT_TRUE(comp->working_pose.positions.empty());
    EXPECT_TRUE(comp->binding.joint_of_track.empty());
  }

  TEST_F(scene_serializer_tests, audio_source_component_play_serialize) {
    scene s("Audio Scene");
    scene_object& emitter = s.create_object("Thruster");

    audio_source_component source = {};
    source.playing = true;
    source.looping = true;
    source.volume = 0.75f;
    source.pitch = 1.25f;
    source.bus = 1;
    source.spatial = true;
    source.min_distance = 2.f;
    source.max_distance = 300.f;
    source.doppler_factor = 0.5f;
    s.add_component<audio_source_component>(&emitter, std::move(source));

    const ostd::vector<uint8_t> snapshot = s.capture_snapshot();

    /// gameplay-style mutations while "playing"
    audio_source_component* live = s.try_get_component<audio_source_component>(emitter.id);
    ASSERT_NE(live, nullptr);
    live->playing = false;
    live->looping = false;
    live->volume = 0.1f;
    /// fake bound runtime state; a restore must never resurrect it
    live->voice = 42;
    live->bound_clip_id = 7;
    live->bound_clip_revision = 3;

    s.restore_snapshot(snapshot);

    scene_object* restored = s.find_object(std::string_view{ "Thruster" });
    ASSERT_NE(restored, nullptr);
    audio_source_component* comp = s.try_get_component<audio_source_component>(restored->id);
    ASSERT_NE(comp, nullptr);

    EXPECT_TRUE(comp->playing);
    EXPECT_TRUE(comp->looping);
    EXPECT_FLOAT_EQ(comp->volume, 0.75f);
    EXPECT_FLOAT_EQ(comp->pitch, 1.25f);
    EXPECT_EQ(comp->bus, 1u);
    EXPECT_FLOAT_EQ(comp->min_distance, 2.f);
    EXPECT_FLOAT_EQ(comp->max_distance, 300.f);
    EXPECT_FLOAT_EQ(comp->doppler_factor, 0.5f);

    /// runtime voice state never serializes; the reconciler rebinds next tick
    EXPECT_EQ(comp->voice, 0u);
    EXPECT_EQ(comp->bound_clip_id, 0u);
    EXPECT_EQ(comp->bound_clip_revision, 0u);
  }

  TEST_F(scene_serializer_tests, snapshot_restore_is_repeatable) {
    scene s("Repeat Scene");
    populate_test_scene(s);

    const ostd::vector<uint8_t> snapshot = s.capture_snapshot();
    for (int i = 0; i < 3; ++i) {
      scene_object& junk = s.create_object(std::format("Junk{}", i));
      (void)junk;
      s.restore_snapshot(snapshot);
      expect_test_scene_state(s);
      EXPECT_EQ(s.get_object_count(), 3u) << "iteration " << i;
    }
  }

}  // namespace other
