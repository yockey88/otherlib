/**
 * \file tests/physics/contact_tests.cpp
 *  doc-03 contacts, queries & joints: begin/end transitions, triggers, raycasts, breakable
 *  welds, runtime body verbs — asserted at the native drain (the C# dispatch is the proven
 *  FixedUpdate invoke path and is exercised live by the editor anchors)
 **/
#include "other_test.hpp"

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"
#include "physics_world/physics_shape.hpp"

#include "object/physics_component.hpp"
#include "object/physics_joint_component.hpp"
#include "scene/scene.hpp"

namespace other {

  class physics_contact_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }

    static physics_component& add_body(scene& s, scene_object& obj, physics_body::type type,
                                       const physics_shape_desc& shape = {}, bool is_trigger = false) {
      physics_body::settings settings;
      settings.body_type = type;
      settings.shape = shape;
      settings.is_trigger = is_trigger;
      return s.add_component<physics_component>(&obj, settings);
    }

    static scene_object& add_floor(scene& s) {
      scene_object& floor = s.create_object("floor", glm::vec3(0.f, 0.f, 0.f));
      physics_shape_desc slab;
      slab.half_extents = { 20.f, 0.5f, 20.f };
      add_body(s, floor, physics_body::STATIC, slab);
      return floor;
    }

    /// tick exactly one fixed step per update and scan that tick's contact transitions
    struct contact_probe {
      bool begin_seen = false;
      bool end_seen = false;
      bool trigger_begin_seen = false;
      bool trigger_end_seen = false;

      void scan(const ostd::vector<contact_event>& events, integer_t body_a, integer_t body_b) {
        for (const contact_event& ev : events) {
          const bool pair = (ev.body_a == body_a && ev.body_b == body_b) || (ev.body_a == body_b && ev.body_b == body_a);
          if (!pair) {
            continue;
          }
          begin_seen |= ev.type == contact_event::kBegin;
          end_seen |= ev.type == contact_event::kEnd;
          trigger_begin_seen |= ev.type == contact_event::kTriggerBegin;
          trigger_end_seen |= ev.type == contact_event::kTriggerEnd;
        }
      }
    };

    static void tick(scene& s, int n = 1) {
      scope<asset_handler> assets = nullptr;
      const double step = subsystem<physics_environment>::get()->get_fixed_step();
      for (int i = 0; i < n; ++i) {
        s.update(step, assets);
      }
    }
  };

  TEST_F(physics_contact_tests, contact_begin_and_end_recorded) {
    scene s("ContactBeginEnd");
    scene_object& floor = add_floor(s);
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 3.f, 0.f));
    physics_component& crate = add_body(s, obj, physics_body::DYNAMIC);
    physics_component* floor_pc = s.get_component<physics_component>(&floor);

    s.play();
    contact_probe probe;
    for (int i = 0; i < 240 && !probe.begin_seen; ++i) {
      tick(s);
      probe.scan(s.last_contact_events(), crate.body->id, floor_pc->body->id);
    }
    EXPECT_TRUE(probe.begin_seen);  /// the crate landed and the listener reported it

    /// launch it back up; separation must produce an end transition
    s.get_storage().physics->add_impulse(crate.body, { 0.f, 10.f, 0.f });
    for (int i = 0; i < 120 && !probe.end_seen; ++i) {
      tick(s);
      probe.scan(s.last_contact_events(), crate.body->id, floor_pc->body->id);
    }
    EXPECT_TRUE(probe.end_seen);
    s.stop();
  }

  TEST_F(physics_contact_tests, trigger_fires_without_response) {
    scene s("TriggerPass");
    physics_shape_desc gate_shape;
    gate_shape.half_extents = { 2.f, 0.5f, 2.f };
    scene_object& gate_obj = s.create_object("gate", glm::vec3(0.f, 2.f, 0.f));
    physics_component& gate = add_body(s, gate_obj, physics_body::STATIC, gate_shape, /*is_trigger=*/ true);

    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 5.f, 0.f));
    physics_component& crate = add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    contact_probe probe;
    for (int i = 0; i < 300; ++i) {
      tick(s);
      probe.scan(s.last_contact_events(), crate.body->id, gate.body->id);
    }

    EXPECT_TRUE(probe.trigger_begin_seen);
    EXPECT_TRUE(probe.trigger_end_seen);                    /// it passed through and left
    EXPECT_FALSE(probe.begin_seen);                          /// never a solid contact
    EXPECT_LT(crate.body->get_current_position().y, 1.f);    /// the sensor did not catch it
    s.stop();
  }

  TEST_F(physics_contact_tests, raycast_hits_and_reports) {
    scene s("RaycastHit");
    scene_object& obj = s.create_object("target", glm::vec3(0.f, 0.f, 0.f));
    add_body(s, obj, physics_body::STATIC);

    raycast_hit hit = s.get_storage().physics->cast_ray({ 0.f, 5.f, 0.f }, { 0.f, -1.f, 0.f }, 10.f);
    ASSERT_TRUE(hit.hit);
    EXPECT_EQ(hit.owner_object_id, obj.id);
    EXPECT_NEAR(hit.point.y, 0.5f, 1e-3f);   /// top face of the unit box
    EXPECT_NEAR(hit.normal.y, 1.f, 1e-3f);
    EXPECT_NEAR(hit.distance, 4.5f, 1e-3f);

    raycast_hit miss = s.get_storage().physics->cast_ray({ 30.f, 5.f, 0.f }, { 0.f, -1.f, 0.f }, 10.f);
    EXPECT_FALSE(miss.hit);
  }

  TEST_F(physics_contact_tests, raycast_ignores_sensors) {
    scene s("RaycastSensor");
    scene_object& solid = s.create_object("solid", glm::vec3(0.f, 0.f, 0.f));
    add_body(s, solid, physics_body::STATIC);

    scene_object& sensor = s.create_object("sensor", glm::vec3(0.f, 2.f, 0.f));
    add_body(s, sensor, physics_body::STATIC, {}, /*is_trigger=*/ true);

    /// the sensor sits between the origin and the solid box; the ray must pass through it
    raycast_hit hit = s.get_storage().physics->cast_ray({ 0.f, 5.f, 0.f }, { 0.f, -1.f, 0.f }, 10.f);
    ASSERT_TRUE(hit.hit);
    EXPECT_EQ(hit.owner_object_id, solid.id);
  }

  TEST_F(physics_contact_tests, joint_holds_under_threshold) {
    scene s("JointHolds");
    scene_object& left = s.create_object("left", glm::vec3(0.f, 10.f, 0.f));
    physics_component& left_pc = add_body(s, left, physics_body::DYNAMIC);
    scene_object& right = s.create_object("right", glm::vec3(1.5f, 10.f, 0.f));
    add_body(s, right, physics_body::DYNAMIC);

    physics_joint_component& weld = s.add_component<physics_joint_component>(&left);
    weld.target_object_name = "right";
    weld.break_force = 1.0e6f;  /// far above anything free fall produces

    s.play();
    tick(s, 120);

    EXPECT_FALSE(weld.broken);
    EXPECT_GE(weld.joint_id, 0);
    EXPECT_LT(left_pc.body->get_current_position().y, 9.f);  /// falling as one welded assembly
    s.stop();
  }

  TEST_F(physics_contact_tests, joint_breaks_over_threshold) {
    scene s("JointBreaks");
    scene_object& anchor = s.create_object("anchor", glm::vec3(0.f, 10.f, 0.f));
    add_body(s, anchor, physics_body::STATIC);

    scene_object& load_obj = s.create_object("load", glm::vec3(0.f, 8.5f, 0.f));
    physics_component& load = add_body(s, load_obj, physics_body::DYNAMIC);
    load.settings.mass = 5.f;

    physics_joint_component& weld = s.add_component<physics_joint_component>(&load_obj);
    weld.target_object_name = "anchor";
    weld.break_force = 10.f;  /// the static load alone is ~49 N (5 kg under gravity)

    s.play();
    for (int i = 0; i < 300 && !weld.broken; ++i) {
      tick(s);
    }

    EXPECT_TRUE(weld.broken);
    EXPECT_EQ(weld.joint_id, -1);
    tick(s, 120);
    EXPECT_LT(load.body->get_current_position().y, 6.f);  /// it tore off and fell away
    s.stop();
  }

  TEST_F(physics_contact_tests, verbs_move_bodies) {
    scene s("BodyVerbs");
    scene_object& obj = s.create_object("crate", glm::vec3(0.f, 10.f, 0.f));
    physics_component& pc = add_body(s, obj, physics_body::DYNAMIC);

    s.play();
    s.get_storage().physics->add_impulse(pc.body, { 5.f, 0.f, 0.f });  /// mass 1 -> 5 m/s
    tick(s);

    glm::vec3 velocity = s.get_storage().physics->get_linear_velocity(pc.body);
    EXPECT_NEAR(velocity.x, 5.f, 0.1f);

    s.get_storage().physics->set_linear_velocity(pc.body, { 0.f, 0.f, -2.f });
    tick(s);
    velocity = s.get_storage().physics->get_linear_velocity(pc.body);
    EXPECT_NEAR(velocity.z, -2.f, 0.1f);
    s.stop();
  }

}  // namespace other
