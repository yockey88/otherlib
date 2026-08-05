/**
 * \file tests/physics/simulation_tests.cpp
 *  doc-01 simulation core: fixed-step cadence, pose seeding, parent-aware write-back,
 *  kinematics, per-world isolation, lifecycle, physics-off tolerance
 **/
#include "other_test.hpp"

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"

#include "object/physics_component.hpp"
#include "scene/scene.hpp"

namespace other {

  class physics_simulation_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }

    static physics_component& add_body(scene& s, scene_object& obj, physics_body::type type, float mass = 1.f) {
      physics_body::settings settings;
      settings.body_type = type;
      settings.mass = mass;
      return s.add_component<physics_component>(&obj, settings);
    }

    static void tick(scene& s, double dt, int n = 1) {
      scope<asset_handler> assets = nullptr;  /// no asset-backed components in these scenes
      for (int i = 0; i < n; ++i) {
        s.update(dt, assets);
      }
    }

    static float body_y(const physics_component& pc) {
      return pc.body->get_current_position().y;
    }

    static double fixed_step() {
      return subsystem<physics_environment>::get()->get_fixed_step();
    }
  };

  TEST_F(physics_simulation_tests, body_spawns_at_entity_pose) {
    scene s("SpawnPose");
    scene_object& obj = s.create_object("box", glm::vec3(10.f, 2.f, -3.f));
    s.get_transform(&obj).set_local_rotation(glm::vec3(0.f, 90.f, 0.f));

    physics_component& pc = add_body(s, obj, physics_body::STATIC);
    ASSERT_NE(pc.body, nullptr);

    glm::vec3 pos = pc.body->get_current_position();
    EXPECT_FLOAT_EQ(pos.x, 10.f);
    EXPECT_FLOAT_EQ(pos.y, 2.f);
    EXPECT_FLOAT_EQ(pos.z, -3.f);
    EXPECT_EQ(pc.body->owner_object_id, obj.id);
  }

  TEST_F(physics_simulation_tests, dynamic_body_falls_deterministically) {
    auto run = [this](const std::string& name) -> float {
      scene s(name);
      scene_object& obj = s.create_object("faller", glm::vec3(0.f, 10.f, 0.f));
      physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);
      s.play();
      tick(s, fixed_step(), 60);
      float y = body_y(pc);
      s.stop();
      return y;
    };

    float first = run("DetFallA");
    float second = run("DetFallB");

    EXPECT_LT(first, 9.f);  /// actually fell under default gravity
    EXPECT_FLOAT_EQ(first, second);
  }

  TEST_F(physics_simulation_tests, feeding_patterns_conserve_simulated_time) {
    /// same wall-clock second delivered in regular vs irregular frame chunks lands on the
    ///  same number of fixed steps, so both bodies end at the same pose
    scene regular("ConserveRegular");
    scene_object& obj_a = regular.create_object("a", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc_a = add_body(regular, obj_a, physics_body::DYNAMIC);

    scene irregular("ConserveIrregular");
    scene_object& obj_b = irregular.create_object("b", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc_b = add_body(irregular, obj_b, physics_body::DYNAMIC);

    regular.play();
    irregular.play();

    tick(regular, 0.01, 100);  /// exactly 1s in 10ms frames

    constexpr std::array<double, 5> kPattern = { 0.003, 0.007, 0.033, 0.017, 0.040 };  /// 100ms per cycle
    for (int cycle = 0; cycle < 10; ++cycle) {                                         /// exactly 1s total
      for (double dt : kPattern) {
        tick(irregular, dt);
      }
    }

    EXPECT_LT(body_y(pc_a), 9.f);
    /// step counts may differ by the sub-step residue at the cut — allow one step of drift
    float one_step_fall = 9.81f * static_cast<float>(fixed_step()) * static_cast<float>(fixed_step()) * 60.f;
    EXPECT_NEAR(body_y(pc_a), body_y(pc_b), one_step_fall);

    regular.stop();
    irregular.stop();
  }

  TEST_F(physics_simulation_tests, catch_up_clamp_bounds_steps) {
    /// a 500ms frame must not free-run the accumulator: it executes exactly the clamp's
    ///  worth of steps, same as a control scene stepped that many times
    scene spike("ClampSpike");
    scene_object& obj_a = spike.create_object("a", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc_a = add_body(spike, obj_a, physics_body::DYNAMIC);

    scene control("ClampControl");
    scene_object& obj_b = control.create_object("b", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc_b = add_body(control, obj_b, physics_body::DYNAMIC);

    spike.play();
    control.play();

    tick(spike, 0.5);              /// one monster frame -> kMaxCatchUpSteps steps + dropped debt
    tick(control, fixed_step(), 5);  /// kMaxCatchUpSteps plain steps

    EXPECT_FLOAT_EQ(body_y(pc_a), body_y(pc_b));

    spike.stop();
    control.stop();
  }

  TEST_F(physics_simulation_tests, presentation_interpolates_between_steps) {
    scene s("Interp");
    scene_object& obj = s.create_object("faller", glm::vec3(0.f, 10.f, 0.f));
    add_body(s, obj, physics_body::DYNAMIC);
    s.play();

    /// one full step: alpha lands on 0, the entity shows the PREVIOUS pose (start)
    tick(s, fixed_step());
    float y_after_step = s.get_transform(&obj).local_position.y;
    EXPECT_NEAR(y_after_step, 10.f, 1e-4f);

    /// half a step: no new simulation, alpha 0.5 blends toward the stepped pose
    tick(s, fixed_step() * 0.5);
    float y_half = s.get_transform(&obj).local_position.y;
    EXPECT_LT(y_half, y_after_step);

    s.stop();
  }

  TEST_F(physics_simulation_tests, parented_body_writes_local_transform) {
    scene s("ParentedFall");
    scene_object& parent = s.create_object("platform", glm::vec3(5.f, 0.f, 0.f));
    scene_object& child = s.create_object("crate", glm::vec3(0.f, 10.f, 0.f), &parent);
    physics_component& pc = add_body(s, child, physics_body::DYNAMIC);

    /// child's WORLD pose includes the parent offset
    EXPECT_FLOAT_EQ(pc.body->get_current_position().x, 5.f);

    s.play();
    tick(s, fixed_step(), 30);

    /// the body fell in world space; the entity's LOCAL transform stays relative to the
    ///  parent (x stays 0 locally while the world pose keeps the parent's x = 5)
    const transform& t = s.get_transform(&child);
    EXPECT_NEAR(t.local_position.x, 0.f, 1e-3f);
    EXPECT_LT(t.local_position.y, 10.f);
    EXPECT_NEAR(pc.body->get_current_position().x, 5.f, 1e-3f);

    s.stop();
  }

  TEST_F(physics_simulation_tests, kinematic_body_follows_entity) {
    scene s("KinematicFollow");
    scene_object& obj = s.create_object("mover", glm::vec3(0.f, 1.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::KINEMATIC);
    s.play();

    for (int i = 0; i < 30; ++i) {
      s.get_transform(&obj).local_position.x += 0.1f;
      tick(s, fixed_step());
    }

    EXPECT_NEAR(pc.body->get_current_position().x, s.get_transform(&obj).local_position.x, 0.2f);
    s.stop();
  }

  TEST_F(physics_simulation_tests, two_worlds_step_independently) {
    scene first("WorldPairA");
    scene_object& obj_a = first.create_object("a", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc_a = add_body(first, obj_a, physics_body::DYNAMIC);

    scene second("WorldPairB");
    scene_object& obj_b = second.create_object("b", glm::vec3(0.f, 20.f, 0.f));
    physics_component& pc_b = add_body(second, obj_b, physics_body::DYNAMIC);

    first.play();
    second.play();

    /// pre-rework this aborted: the backend iterated one global body map across worlds
    tick(first, fixed_step(), 30);
    tick(second, fixed_step(), 30);

    EXPECT_LT(body_y(pc_a), 10.f);
    EXPECT_LT(body_y(pc_b), 20.f);

    first.stop();
    second.stop();
  }

  TEST_F(physics_simulation_tests, world_recreate_is_fresh) {
    /// same scene name -> same FNV scene id -> same backend world id; pre-rework the stale
    ///  world (never shut down) was silently resurrected
    {
      scene s("RecreateMe");
      scene_object& obj = s.create_object("box", glm::vec3(0.f, 10.f, 0.f));
      add_body(s, obj, physics_body::DYNAMIC);
      s.play();
      tick(s, fixed_step(), 10);
      s.stop();
    }

    scene reborn("RecreateMe");
    scene_object& obj = reborn.create_object("box", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc = add_body(reborn, obj, physics_body::DYNAMIC);
    reborn.play();
    tick(reborn, fixed_step(), 10);
    EXPECT_LT(body_y(pc), 10.f);
    reborn.stop();
  }

  TEST_F(physics_simulation_tests, play_stop_restores_body_pose) {
    scene s("PlayStopRestore");
    scene_object& obj = s.create_object("faller", glm::vec3(0.f, 10.f, 0.f));
    add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    tick(s, fixed_step(), 30);
    EXPECT_LT(s.get_transform(&obj).local_position.y, 10.f);

    s.stop();
    s.reset();

    /// snapshot restore rebuilt the object; re-resolve and check both entity and body pose
    scene_object* restored = s.find_object("faller");
    ASSERT_NE(restored, nullptr);
    EXPECT_NEAR(s.get_transform(restored).local_position.y, 10.f, 1e-4f);

    physics_component* pc = s.get_component<physics_component>(restored);
    ASSERT_NE(pc, nullptr);
    ASSERT_NE(pc->body, nullptr);
    EXPECT_NEAR(pc->body->get_current_position().y, 10.f, 1e-4f);
    /// NB the restored body is default-settings until doc-02's settings serialization +
    ///  revalidation land, so "falls again identically" is asserted there, not here
  }

  TEST_F(physics_simulation_tests, physics_off_scene_is_legal) {
    /// the harness force-activates every subsystem; re-inert physics for this test's scope
    subsystem<physics_environment>::inert = true;

    {
      scene s("NoPhysics");
      scene_object& obj = s.create_object("box", glm::vec3(0.f, 10.f, 0.f));
      physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);
      EXPECT_EQ(pc.body, nullptr);  /// component stays inert data

      s.play();
      tick(s, 1.0 / 60.0, 5);  /// literal step: the subsystem is inert, get_fixed_step is unreachable
      EXPECT_FLOAT_EQ(s.get_transform(&obj).local_position.y, 10.f);
      s.stop();
    }

    subsystem<physics_environment>::inert = false;
  }

}  // namespace other
