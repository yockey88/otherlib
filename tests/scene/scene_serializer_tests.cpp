/**
 * \file tests/scene/scene_serializer_tests.cpp
 *
 * live scene <-> document conversions and the snapshot store/restore primitive that
 * play/stop restore and editor undo/redo ride on.
 **/
#include <gtest/gtest.h>

#include "other_test.hpp"

#include "object/grid_component.hpp"
#include "object/light_component.hpp"
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
