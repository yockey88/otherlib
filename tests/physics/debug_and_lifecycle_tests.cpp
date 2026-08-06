/**
 * \file tests/physics/debug_and_lifecycle_tests.cpp
 *  doc-04: the debug-render pull path headless, the roadmap bullet-6 lifecycle sentence,
 *  world teardown through scene destruction, pool exhaustion by contract
 **/
#include "other_test.hpp"

#include "physics/physics_environment.hpp"
#include "physics_world/physics_body.hpp"
#include "physics_world/physics_shape.hpp"

#include "object/physics_component.hpp"
#include "scene/scene.hpp"

namespace other {

  class physics_debug_lifecycle_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }

    static physics_component& add_body(scene& s, scene_object& obj, physics_body::type type,
                                       const physics_shape_desc& shape = {}) {
      physics_body::settings settings;
      settings.body_type = type;
      settings.shape = shape;
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

  TEST_F(physics_debug_lifecycle_tests, debug_data_flows_headless) {
    scene s("DebugData");
    scene_object& obj = s.create_object("box", glm::vec3(0.f, 0.f, 0.f));
    add_body(s, obj, physics_body::STATIC);

    physics_api::physics_render_debug_data data = s.get_storage().physics->get_debug_render_data();
    EXPECT_FALSE(data.debug_lines.empty());  /// wireframe box edges (JPH_DEBUG_RENDERER linkage proof)
    EXPECT_EQ(data.debug_lines.size(), data.debug_line_colors.size());
    EXPECT_EQ(data.debug_triangles.size(), data.debug_triangle_colors.size());
  }

  TEST_F(physics_debug_lifecycle_tests, debug_data_empty_world) {
    scene s("DebugEmpty");
    physics_api::physics_render_debug_data data = s.get_storage().physics->get_debug_render_data();
    EXPECT_TRUE(data.debug_lines.empty());
    EXPECT_TRUE(data.debug_triangles.empty());
  }

  TEST_F(physics_debug_lifecycle_tests, spawn_step_collide_teardown) {
    /// roadmap phase-4 bullet 6, as one sentence
    {
      scene s("Bullet6");
      physics_shape_desc slab;
      slab.half_extents = { 10.f, 0.5f, 10.f };
      scene_object& floor = s.create_object("floor", glm::vec3(0.f, 0.f, 0.f));
      physics_component* floor_pc = &add_body(s, floor, physics_body::STATIC, slab);

      scene_object& obj = s.create_object("crate", glm::vec3(0.f, 3.f, 0.f));
      physics_component& crate = add_body(s, obj, physics_body::DYNAMIC);

      s.play();
      bool collided = false;
      for (int i = 0; i < 240 && !collided; ++i) {
        tick(s);
        for (const contact_event& ev : s.last_contact_events()) {
          collided |= ev.type == contact_event::kBegin &&
            ((ev.body_a == crate.body->id && ev.body_b == floor_pc->body->id) ||
             (ev.body_b == crate.body->id && ev.body_a == floor_pc->body->id));
        }
      }
      EXPECT_TRUE(collided);
      s.stop();
    }  /// teardown: scene dtor drains bodies, destroy_world shuts the backend world down

    scene reborn("Bullet6");  /// same name, same world id — must come back fresh
    scene_object& obj = reborn.create_object("crate", glm::vec3(0.f, 3.f, 0.f));
    physics_component& pc = add_body(reborn, obj, physics_body::DYNAMIC);
    reborn.play();
    tick(reborn, 60);
    EXPECT_LT(pc.body->get_current_position().y, 3.f);
    reborn.stop();
  }

  TEST_F(physics_debug_lifecycle_tests, worlds_shut_down_through_scene_destruction) {
    {
      scene first("TeardownA");
      scene_object& a = first.create_object("a", glm::vec3(0.f, 5.f, 0.f));
      add_body(first, a, physics_body::DYNAMIC);

      scene second("TeardownB");
      scene_object& b = second.create_object("b", glm::vec3(0.f, 5.f, 0.f));
      add_body(second, b, physics_body::DYNAMIC);

      first.play();
      second.play();
      tick(first, 10);
      tick(second, 10);
      first.stop();
      second.stop();
    }

    /// both worlds died cleanly; recreating both names yields fresh functional worlds
    scene first("TeardownA");
    scene second("TeardownB");
    scene_object& a = first.create_object("a", glm::vec3(0.f, 5.f, 0.f));
    physics_component& pc = add_body(first, a, physics_body::DYNAMIC);
    first.play();
    tick(first, 30);
    EXPECT_LT(pc.body->get_current_position().y, 5.f);
    first.stop();
  }

  TEST_F(physics_debug_lifecycle_tests, pool_exhaustion_fails_loudly) {
    scene s("PoolExhaustion");
    physics_world* world = s.get_storage().physics;

    /// fill the global body pool through the direct API; the last create must fail by
    ///  contract (nullptr + error log) instead of aborting in the pool
    std::vector<physics_body*> bodies;
    physics_body::settings settings;
    settings.body_type = physics_body::STATIC;
    for (int i = 0; i < 3000; ++i) {
      physics_body* body = world->create_physics_body(settings, glm::mat4(1.f));
      if (body == nullptr) {
        break;
      }
      bodies.push_back(body);
    }

    EXPECT_LT(bodies.size(), 3000u);                                             /// it did run out
    EXPECT_EQ(world->create_physics_body(settings, glm::mat4(1.f)), nullptr);    /// and stays out

    for (physics_body* body : bodies) {
      world->destroy_physics_body(body);
    }
    EXPECT_NE(world->create_physics_body(settings, glm::mat4(1.f)), nullptr);    /// slots recycle
  }

}  // namespace other
