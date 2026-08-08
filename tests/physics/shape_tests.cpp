/**
 * \file tests/physics/shape_tests.cpp
 *  doc-02 shapes & colliders: desc-driven builds reach the backend, every kind collides,
 *  revalidation reconciles settings/shape/scale edits and restored bodies
 **/
#include "other_test.hpp"

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"
#include "physics_world/physics_shape.hpp"

#include "object/physics_component.hpp"
#include "scene/scene.hpp"

namespace other {

  class physics_shape_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }

    static physics_component& add_body(scene& s, scene_object& obj, physics_body::type type,
                                       const physics_shape_desc& shape = {}) {
      physics_body::settings settings;
      settings.body_type = type;
      settings.shape = shape;
      return s.add_component<physics_component>(&obj, settings);
    }

    static scene_object& add_floor(scene& s) {
      scene_object& floor = s.create_object("floor", glm::vec3(0.f, 0.f, 0.f));
      physics_shape_desc slab;
      slab.shape_kind = PHYSICS_SHAPE_BOX;
      slab.half_extents = { 20.f, 0.5f, 20.f };
      add_body(s, floor, physics_body::STATIC, slab);
      return floor;
    }

    static void tick(scene& s, int n = 1) {
      scope<asset_handler> assets = nullptr;
      const double step = subsystem<physics_environment>::get()->get_fixed_step();
      for (int i = 0; i < n; ++i) {
        s.update(step, assets);
      }
    }

    static float body_y(const physics_component& pc) {
      return pc.body->get_current_position().y;
    }

    /// unit cube point cloud / triangle list for direct hull/mesh builds (no render model needed)
    static void cube_geometry(ostd::vector<glm::vec3>& positions, ostd::vector<uint32_t>& indices, float half = 0.5f) {
      positions = {
        { -half, -half, -half }, { half, -half, -half }, { half, half, -half }, { -half, half, -half },
        { -half, -half, half }, { half, -half, half }, { half, half, half }, { -half, half, half },
      };
      indices = {
        0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 0, 4, 5, 0, 5, 1,
        3, 2, 6, 3, 6, 7, 0, 3, 7, 0, 7, 4, 1, 5, 6, 1, 6, 2,
      };
    }
  };

  TEST_F(physics_shape_tests, box_shape_reaches_jolt) {
    scene s("BoxReachesJolt");
    add_floor(s);
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 5.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);  /// default desc = unit box

    s.play();
    tick(s, 240);

    /// pre-doc-02 every body kept its empty creation shape and fell forever
    EXPECT_NEAR(body_y(pc), 1.f, 0.1f);  /// floor top 0.5 + box half extent 0.5
    s.stop();
  }

  TEST_F(physics_shape_tests, stacked_boxes_rest) {
    scene s("StackedBoxes");
    add_floor(s);

    std::array<physics_component*, 3> crates = {};
    for (int i = 0; i < 3; ++i) {
      scene_object& obj = s.create_object(std::format("crate-{}", i), glm::vec3(0.f, 1.2f + 1.3f * i, 0.f));
      crates[i] = &add_body(s, obj, physics_body::DYNAMIC);
    }

    s.play();
    tick(s, 420);

    std::array<float, 3> settled = { body_y(*crates[0]), body_y(*crates[1]), body_y(*crates[2]) };
    EXPECT_NEAR(settled[0], 1.f, 0.15f);
    EXPECT_NEAR(settled[1], 2.f, 0.2f);
    EXPECT_NEAR(settled[2], 3.f, 0.25f);

    tick(s, 60);  /// still resting, not slowly interpenetrating or jittering apart
    EXPECT_NEAR(body_y(*crates[0]), settled[0], 0.05f);
    EXPECT_NEAR(body_y(*crates[2]), settled[2], 0.05f);
    s.stop();
  }

  TEST_F(physics_shape_tests, sphere_and_capsule_rest_at_their_radii) {
    scene s("RoundShapes");
    add_floor(s);

    physics_shape_desc sphere;
    sphere.shape_kind = PHYSICS_SHAPE_SPHERE;
    sphere.radius = 0.5f;
    scene_object& ball = s.create_object("ball", glm::vec3(-3.f, 5.f, 0.f));
    physics_component& ball_pc = add_body(s, ball, physics_body::DYNAMIC, sphere);

    physics_shape_desc capsule;
    capsule.shape_kind = PHYSICS_SHAPE_CAPSULE;
    capsule.radius = 0.5f;
    capsule.half_height = 0.5f;
    scene_object& pill = s.create_object("pill", glm::vec3(3.f, 5.f, 0.f));
    physics_component& pill_pc = add_body(s, pill, physics_body::DYNAMIC, capsule);

    s.play();
    tick(s, 300);

    EXPECT_NEAR(body_y(ball_pc), 1.f, 0.1f);  /// floor top + radius
    EXPECT_NEAR(body_y(pill_pc), 1.5f, 0.15f);  /// floor top + half_height + cap radius (upright)
    s.stop();
  }

  TEST_F(physics_shape_tests, hull_and_mesh_build_from_geometry) {
    scene s("GeometryShapes");

    /// static mesh floor built from raw triangles (physics_world entry point takes spans
    ///  directly); jolt triangles are single-sided, so the fixture emits both windings
    ostd::vector<glm::vec3> positions = {
      { -20.f, 0.f, -20.f }, { 20.f, 0.f, -20.f }, { 20.f, 0.f, 20.f }, { -20.f, 0.f, 20.f },
    };
    ostd::vector<uint32_t> indices = {
      0, 2, 1, 0, 3, 2,
      0, 1, 2, 0, 2, 3,
    };

    physics_shape_desc mesh_desc;
    mesh_desc.shape_kind = PHYSICS_SHAPE_TRIANGLE_MESH;
    scene_object& ground = s.create_object("mesh-ground", glm::vec3(0.f, 0.f, 0.f));
    physics_component& ground_pc = add_body(s, ground, physics_body::STATIC, mesh_desc);
    shape_geometry ground_geo{ positions, indices };
    s.get_storage().physics->apply_shape(ground_pc.body, mesh_desc, glm::vec3(1.f), &ground_geo);
    ASSERT_NE(ground_pc.body->shape_id, -1);

    /// dynamic hull crate from the same cube points
    ostd::vector<glm::vec3> hull_points;
    ostd::vector<uint32_t> hull_indices;
    cube_geometry(hull_points, hull_indices, 0.5f);

    physics_shape_desc hull_desc;
    hull_desc.shape_kind = PHYSICS_SHAPE_CONVEX_HULL;
    scene_object& crate = s.create_object("hull-crate", glm::vec3(0.f, 3.f, 0.f));
    physics_component& crate_pc = add_body(s, crate, physics_body::DYNAMIC, hull_desc);
    shape_geometry crate_geo{ hull_points, hull_indices };
    physics_shape* crate_shape = s.get_storage().physics->apply_shape(crate_pc.body, hull_desc, glm::vec3(1.f), &crate_geo);

    ASSERT_NE(crate_shape, nullptr);
    EXPECT_FALSE(crate_shape->get_bounding_box() == bounding_box::empty);

    s.play();
    tick(s, 300);

    /// the hull crate rests its half extent above the y=0 mesh floor
    EXPECT_NEAR(body_y(crate_pc), 0.5f, 0.1f);
    s.stop();
  }

  TEST_F(physics_shape_tests, mesh_on_dynamic_downgrades_to_hull) {
    scene s("MeshDowngrade");
    add_floor(s);

    ostd::vector<glm::vec3> positions;
    ostd::vector<uint32_t> indices;
    cube_geometry(positions, indices, 0.5f);

    physics_shape_desc mesh_desc;
    mesh_desc.shape_kind = PHYSICS_SHAPE_TRIANGLE_MESH;  /// illegal on a dynamic body
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 5.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC, mesh_desc);
    shape_geometry geo{ positions, indices };
    s.get_storage().physics->apply_shape(pc.body, mesh_desc, glm::vec3(1.f), &geo);

    s.play();
    tick(s, 240);

    /// warned + built as a hull: it collides and rests instead of asserting or tunneling
    EXPECT_NEAR(body_y(pc), 1.f, 0.1f);
    s.stop();
  }

  TEST_F(physics_shape_tests, desc_edit_rebuilds_mid_play) {
    scene s("DescEdit");
    add_floor(s);
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 3.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    tick(s, 240);
    EXPECT_NEAR(body_y(pc), 1.f, 0.1f);

    pc.settings.shape.half_extents = { 0.5f, 1.0f, 0.5f };  /// grow the box; revalidation rebuilds
    tick(s, 240);
    EXPECT_NEAR(body_y(pc), 1.5f, 0.15f);  /// floor top + new half extent
    s.stop();
  }

  TEST_F(physics_shape_tests, body_settings_edit_rebuilds_mid_play) {
    scene s("BodyEdit");
    add_floor(s);
    scene_object& obj = s.create_object("statue", glm::vec3(0.f, 5.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::STATIC);

    s.play();
    tick(s, 60);
    EXPECT_NEAR(body_y(pc), 5.f, 1e-3f);  /// static: hangs in the air

    pc.settings.body_type = physics_body::DYNAMIC;  /// authored != applied -> body recreated
    tick(s, 240);
    EXPECT_NEAR(body_y(pc), 1.f, 0.1f);  /// now it falls and rests
    s.stop();
  }

  TEST_F(physics_shape_tests, scale_bakes_into_shape) {
    scene s("ScaleBake");
    add_floor(s);
    scene_object& obj = s.create_object("bigcrate", glm::vec3(0.f, 5.f, 0.f));
    s.get_transform(&obj).local_scale = glm::vec3(2.f);
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    tick(s, 240);
    EXPECT_NEAR(body_y(pc), 1.5f, 0.1f);  /// floor top + 0.5 half extent * 2 scale
    s.stop();
  }

  TEST_F(physics_shape_tests, settings_survive_snapshot_restore) {
    scene s("SettingsRoundTrip");
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 5.f, 0.f));

    physics_body::settings authored;
    authored.body_type = physics_body::DYNAMIC;
    authored.mass = 3.5f;
    authored.friction = 0.9f;
    authored.restitution = 0.25f;
    authored.gravity_factor = 0.5f;
    authored.is_trigger = false;
    authored.continuous_cd = true;
    authored.shape.shape_kind = PHYSICS_SHAPE_SPHERE;
    authored.shape.radius = 1.25f;
    physics_component& pc = s.add_component<physics_component>(&obj, authored);

    s.play();  /// captures the snapshot
    pc.settings = physics_body::settings{};  /// stomp everything mid-play
    s.stop();
    s.reset();  /// restore

    scene_object* restored = s.find_object("crate");
    ASSERT_NE(restored, nullptr);
    physics_component* restored_pc = s.get_component<physics_component>(restored);
    ASSERT_NE(restored_pc, nullptr);
    EXPECT_TRUE(restored_pc->settings == authored);  /// every reflected field round-tripped
  }

  TEST_F(physics_shape_tests, restored_body_regains_settings) {
    scene s("RestoreRegains");
    add_floor(s);
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 5.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    tick(s, 120);
    EXPECT_LT(body_y(pc), 5.f);
    s.stop();
    s.reset();

    /// the doc-01 deferred anchor: the restored body was default-built (STATIC) by the
    ///  construct hook; play-time revalidation must rebuild it DYNAMIC so it falls again
    scene_object* restored = s.find_object("crate");
    ASSERT_NE(restored, nullptr);
    physics_component* restored_pc = s.get_component<physics_component>(restored);
    ASSERT_NE(restored_pc, nullptr);

    s.play();
    tick(s, 120);
    EXPECT_LT(restored_pc->body->get_current_position().y, 4.5f);
    s.stop();
  }

  TEST_F(physics_shape_tests, shape_bounds_feed_scene_aabb) {
    scene s("ShapeBounds");
    physics_shape_desc sphere;
    sphere.shape_kind = PHYSICS_SHAPE_SPHERE;
    sphere.radius = 2.f;
    scene_object& obj = s.create_object("ball", glm::vec3(0.f, 0.f, 0.f));
    add_body(s, obj, physics_body::STATIC, sphere);

    bounding_box box = s.get_bounding_box(&obj);
    EXPECT_NEAR(box.min.y, -2.f, 1e-3f);
    EXPECT_NEAR(box.max.y, 2.f, 1e-3f);
  }

}  // namespace other
