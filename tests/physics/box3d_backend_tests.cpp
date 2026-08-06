/**
 * \file tests/physics/box3d_backend_tests.cpp
 *  the backend A/B: the same engine-facing scenarios that run under jolt, executed against
 *  box3d by swapping the backend on the live physics_environment
 **/
#include "other_test.hpp"

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"
#include "physics_world/physics_shape.hpp"

#include "object/physics_component.hpp"
#include "object/physics_joint_component.hpp"
#include "scene/scene.hpp"

namespace other {

  class box3d_backend_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }

    /// swap the live environment onto box3d for the guard's scope; scenes must live INSIDE
    ///  the guard (worlds bind to the backend they were created under)
    struct backend_swap {
      backend_swap() {
        swap_to("[physics]\nbackend = \"box3d\"\n");
      }
      ~backend_swap() {
        swap_to("[physics]\nbackend = \"jolt\"\n");
      }

      static void swap_to(const std::string_view config_text) {
        auto* env = subsystem<physics_environment>::get();
        OTHER_ASSERT(env != nullptr, "Physics environment is not initialized.");
        opt<config_table> config = parse_string_config(config_text);
        OTHER_ASSERT(config.has_value(), "Failed to parse backend swap config.");
        env->unload_backend();
        env->load_backend(*config);
      }
    };

    static physics_component& add_body(scene& s, scene_object& obj, physics_body::type type,
                                       const physics_shape_desc& shape = {}, bool is_trigger = false) {
      physics_body::settings settings;
      settings.body_type = type;
      settings.shape = shape;
      settings.is_trigger = is_trigger;
      return s.add_component<physics_component>(&obj, settings);
    }

    static void tick(scene& s, int n = 1) {
      scope<asset_handler> assets = nullptr;
      const double step = subsystem<physics_environment>::get()->get_fixed_step();
      for (int i = 0; i < n; ++i) {
        s.update(step, assets);
      }
    }
  };

  TEST_F(box3d_backend_tests, box3d_end_to_end) {
    backend_swap swap;
    {
      scene s("Box3dEndToEnd");

      physics_shape_desc slab;
      slab.half_extents = { 20.f, 0.5f, 20.f };
      scene_object& floor = s.create_object("floor", glm::vec3(0.f, 0.f, 0.f));
      physics_component& floor_pc = add_body(s, floor, physics_body::STATIC, slab);

      scene_object& crate_obj = s.create_object("crate", glm::vec3(0.f, 3.f, 0.f));
      physics_component& crate = add_body(s, crate_obj, physics_body::DYNAMIC);

      s.play();

      /// falls, collides (contact event observed), and rests on the floor
      bool contact_seen = false;
      for (int i = 0; i < 300; ++i) {
        tick(s);
        for (const contact_event& ev : s.last_contact_events()) {
          contact_seen |= ev.type == contact_event::kBegin &&
            ((ev.body_a == crate.body->id && ev.body_b == floor_pc.body->id) ||
             (ev.body_b == crate.body->id && ev.body_a == floor_pc.body->id));
        }
      }
      EXPECT_TRUE(contact_seen);
      EXPECT_NEAR(crate.body->get_current_position().y, 1.f, 0.15f);

      /// raycast reports the crate's top face
      raycast_hit hit = s.get_storage().physics->cast_ray({ 0.f, 5.f, 0.f }, { 0.f, -1.f, 0.f }, 10.f);
      ASSERT_TRUE(hit.hit);
      EXPECT_EQ(hit.owner_object_id, crate_obj.id);

      /// verbs: an impulse moves the authored-mass body at the expected speed
      s.get_storage().physics->add_impulse(crate.body, { 5.f, 0.f, 0.f });
      tick(s);
      EXPECT_NEAR(s.get_storage().physics->get_linear_velocity(crate.body).x, 5.f, 0.25f);

      s.stop();
    }
  }

  TEST_F(box3d_backend_tests, box3d_weld_breaks) {
    backend_swap swap;
    {
      scene s("Box3dWeld");
      scene_object& anchor = s.create_object("anchor", glm::vec3(0.f, 10.f, 0.f));
      add_body(s, anchor, physics_body::STATIC);

      scene_object& load_obj = s.create_object("load", glm::vec3(0.f, 8.5f, 0.f));
      physics_component& load = add_body(s, load_obj, physics_body::DYNAMIC);
      load.settings.mass = 5.f;

      physics_joint_component& weld = s.add_component<physics_joint_component>(&load_obj);
      weld.target_object_name = "anchor";
      weld.break_force = 10.f;  /// the static load alone is ~49 N

      s.play();
      for (int i = 0; i < 300 && !weld.broken; ++i) {
        tick(s);
      }
      EXPECT_TRUE(weld.broken);
      tick(s, 120);
      EXPECT_LT(load.body->get_current_position().y, 6.f);
      s.stop();
    }
  }

}  // namespace other
