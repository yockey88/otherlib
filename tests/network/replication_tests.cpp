/**
 * \file network/replication_tests.cpp
 *  M4: the real session/replication stack over memory links — two headless scenes,
 *  one sim mesh, manual step, virtual clock. one end-to-end test rides tcp.
 **/
#include <algorithm>

#include <gtest/gtest.h>

#include "object/network_component.hpp"
#include "object/physics_component.hpp"
#include "object/transform.hpp"
#include "physics_world/physics_body.hpp"
#include "scene/scene.hpp"

#include "network/session/net_messages.hpp"
#include "network/session/network_session.hpp"
#include "network/session/replication.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "network/mesh_sim_fixture.hpp"
#include "network/socket_mesh_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class replication_tests : public other_test {
   protected:
    bool script_and_physics() const override { return true; }
  };

  namespace {

    peer_mesh_config fast_cfg() {
      peer_mesh_config cfg;
      cfg.handshake_timeout = microseconds{ 500'000 };
      cfg.keepalive_idle = microseconds{ 250'000 };
      cfg.link_timeout = microseconds{ 1'000'000 };
      return cfg;
    }

    network_session::session_config fast_scfg(std::string name) {
      network_session::session_config cfg;
      cfg.join_timeout = microseconds{ 400'000 };
      cfg.display_name = std::move(name);
      return cfg;
    }

    /// one sim mesh, host + client seat, scene + replication stack per seat; dtor clears
    ///  session observers before members unwind (mesh teardown fires ENDED first)
    struct repl_sim {
      mesh_sim_fixture sim;
      scene host_scene{ "host-world" };
      scene client_scene{ "client-world" };
      network_session* host_session = nullptr;
      network_session* client_session = nullptr;
      scope<replication> host_repl;
      scope<replication> client_repl;
      uint64_t endpoint = 0;

      explicit repl_sim(const replication_config& repl_cfg = {},
                        const peer_mesh_config& mesh_cfg = fast_cfg(),
                        const network_session::session_config& session_cfg = fast_scfg("peer"))
          : sim(0, 1, mesh_cfg) {
        host_session = &spawn(1, session_cfg);
        client_session = &spawn(2, session_cfg);
        host_repl = make_scope<replication>(*host_session, [this] { return &host_scene; }, repl_cfg);
        client_repl = make_scope<replication>(*client_session, [this] { return &client_scene; }, repl_cfg);
        host_session->set_observer([this](session_event ev, uint16_t arg) { host_repl->on_session_event(ev, arg); });
        client_session->set_observer([this](session_event ev, uint16_t arg) { client_repl->on_session_event(ev, arg); });

        endpoint = sim.next_endpoint++;
        sim.fabric.configure_endpoint(endpoint, false);
      }

      ~repl_sim() {
        host_session->set_observer(nullptr);
        client_session->set_observer(nullptr);
      }

      network_session& spawn(node_id node, const network_session::session_config& cfg) {
        return static_cast<network_session&>(sim.spawn(make_scope<network_session>(cfg), node));
      }

      void step(size_t ticks = 1, microseconds dt = microseconds{ 5'000 }) {
        for (size_t i = 0; i < ticks; ++i) {
          sim.step(dt);
          host_repl->tick(sim.now);
          client_repl->tick(sim.now);
        }
      }

      template <typename Pred>
      bool step_until(Pred&& done, size_t max_ticks = 512, microseconds dt = microseconds{ 5'000 }) {
        for (size_t i = 0; i < max_ticks; ++i) {
          if (done()) {
            return true;
          }
          step(1, dt);
        }
        return done();
      }

      bool connect() {
        if (!host_session->host(net_address::memory_endpoint(endpoint)) ||
            !client_session->join(net_address::memory_endpoint(endpoint))) {
          return false;
        }
        return step_until([&] { return client_session->in_session(); });
      }
    };

    scene_object& add_cube(scene& s, const std::string& name, const glm::vec3& position, scene_object* parent = nullptr) {
      scene_object& object = s.create_object(name, parent);
      s.get_component<transform>(&object)->local_position = position;
      return object;
    }

    float client_x(repl_sim& fx, const std::string& name) {
      scene_object* object = fx.client_scene.find_object(name);
      return object != nullptr ? fx.client_scene.get_component<transform>(object)->local_position.x : -10'000.f;
    }

  }  // namespace

  TEST_F(replication_tests, join_snapshot_reproduces_host_scene) {
    repl_sim fx;
    scene_object& floor = add_cube(fx.host_scene, "floor", { 0.f, -1.f, 0.f });
    add_cube(fx.host_scene, "child", { 0.f, 2.f, 0.f }, &floor);
    scene_object& donut = add_cube(fx.host_scene, "donut", { 3.f, 5.f, 0.f });
    fx.host_scene.add_component<network_component>(&donut);

    ASSERT_TRUE(fx.connect());
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("donut") != nullptr; }));

    EXPECT_EQ(fx.client_scene.get_object_count(), fx.host_scene.get_object_count());
    EXPECT_EQ(fx.client_scene.network().role, replication_role::REPLICA);
    EXPECT_EQ(fx.host_scene.network().role, replication_role::AUTHORITY);

    /// hierarchy and fields survive (runtime ids excluded by construction)
    scene_object* client_child = fx.client_scene.find_object("child");
    ASSERT_NE(client_child, nullptr);
    const scene_object* client_parent = fx.client_scene.get_parent(client_child->id);
    ASSERT_NE(client_parent, nullptr);
    EXPECT_EQ(client_parent->name, "floor");
    EXPECT_FLOAT_EQ(client_x(fx, "donut"), 3.f);

    /// the authored object landed in both registries under one net id
    const opt<natural_t> host_net = fx.host_scene.network().net_of(donut.id);
    ASSERT_TRUE(host_net.has_value());
    const opt<natural_t> client_object = fx.client_scene.network().object_of(*host_net);
    ASSERT_TRUE(client_object.has_value());
    EXPECT_EQ(fx.client_scene.find_object(*client_object)->name, "donut");
  }

  TEST_F(replication_tests, spawn_after_join_materializes_with_components) {
    repl_sim fx;
    ASSERT_TRUE(fx.connect());

    scene_object& crate = add_cube(fx.host_scene, "crate", { 1.f, 2.f, 3.f });
    physics_body::settings body;
    body.body_type = physics_body::DYNAMIC;
    fx.host_scene.add_component<physics_component>(&crate, body);
    ASSERT_TRUE(fx.host_repl->spawn_object(crate.id));

    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("crate") != nullptr; }));
    scene_object* replica = fx.client_scene.find_object("crate");
    EXPECT_FLOAT_EQ(fx.client_scene.get_component<transform>(replica)->local_position.z, 3.f);
    EXPECT_TRUE(fx.client_scene.has_component<physics_component>(replica));
  }

  TEST_F(replication_tests, despawn_removes_and_forgets) {
    repl_sim fx;
    ASSERT_TRUE(fx.connect());

    scene_object& crate = add_cube(fx.host_scene, "crate", { 0.f, 0.f, 0.f });
    ASSERT_TRUE(fx.host_repl->spawn_object(crate.id));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("crate") != nullptr; }));
    const natural_t net_id = *fx.host_scene.network().net_of(crate.id);

    /// plain destroy replicates: the sweep is the despawn detector
    fx.host_scene.destroy_object(crate.id);
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("crate") == nullptr; }));
    EXPECT_FALSE(fx.client_scene.network().object_of(net_id).has_value());
    EXPECT_FALSE(fx.host_scene.network().object_of(net_id).has_value());
  }

  TEST_F(replication_tests, transform_batch_interpolates_between_samples) {
    repl_sim fx;
    ASSERT_TRUE(fx.connect());

    scene_object& mover = add_cube(fx.host_scene, "mover", { 0.f, 0.f, 0.f });
    ASSERT_TRUE(fx.host_repl->spawn_object(mover.id));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("mover") != nullptr; }));

    /// host slides +1 x per snapshot interval; the replica must move through
    ///  sub-sample positions rather than snapping sample to sample
    transform* host_trs = fx.host_scene.get_component<transform>(&mover);
    ostd::vector<float> observed;
    for (size_t i = 0; i < 120; ++i) {
      host_trs->local_position.x = static_cast<float>(fx.host_repl->host_tick());
      fx.step(1);
      observed.push_back(client_x(fx, "mover"));
    }

    float max_step = 0.f;
    float total = 0.f;
    for (size_t i = 60; i < observed.size(); ++i) {  // past warmup
      const float delta = observed[i] - observed[i - 1];
      EXPECT_GE(delta, -1e-3f);  // never moves backwards
      max_step = std::max(max_step, delta);
      total += delta;
    }
    EXPECT_GT(total, 2.f);       // it is actually tracking the host
    EXPECT_LT(max_step, 0.75f);  // and never jumps a full sample step
    /// sub-sample positions prove lerp: some observed x must be strictly between ticks
    const bool fractional = std::ranges::any_of(observed, [](float x) {
      return x > 0.f && glm::abs(x - glm::round(x)) > 0.05f;
    });
    EXPECT_TRUE(fractional);
  }

  TEST_F(replication_tests, replica_dynamic_bodies_register_kinematic) {
    repl_sim fx;
    scene_object& donut = add_cube(fx.host_scene, "donut", { 0.f, 5.f, 0.f });
    physics_body::settings body;
    body.body_type = physics_body::DYNAMIC;
    fx.host_scene.add_component<physics_component>(&donut, body);
    fx.host_scene.add_component<network_component>(&donut);

    ASSERT_TRUE(fx.connect());
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("donut") != nullptr; }));

    physics_component* host_body = fx.host_scene.get_component<physics_component>(&donut);
    ASSERT_NE(host_body->body, nullptr);
    EXPECT_EQ(host_body->body->body_type, physics_body::DYNAMIC);

    scene_object* replica = fx.client_scene.find_object("donut");
    physics_component* replica_body = fx.client_scene.get_component<physics_component>(replica);
    ASSERT_NE(replica_body, nullptr);
    ASSERT_NE(replica_body->body, nullptr);
    EXPECT_EQ(replica_body->body->body_type, physics_body::KINEMATIC);
    /// authored settings untouched — role is runtime state
    EXPECT_EQ(replica_body->settings.body_type, physics_body::DYNAMIC);
  }

  TEST_F(replication_tests, session_end_restores_authored_body_type) {
    repl_sim fx;
    scene_object& donut = add_cube(fx.host_scene, "donut", { 0.f, 5.f, 0.f });
    physics_body::settings body;
    body.body_type = physics_body::DYNAMIC;
    fx.host_scene.add_component<physics_component>(&donut, body);
    fx.host_scene.add_component<network_component>(&donut);

    ASSERT_TRUE(fx.connect());
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("donut") != nullptr; }));

    fx.client_session->leave();
    fx.step(4);
    EXPECT_EQ(fx.client_scene.network().role, replication_role::NONE);

    /// replicas stay as local objects; bodies revalidate to authored settings
    scene_object* orphan = fx.client_scene.find_object("donut");
    ASSERT_NE(orphan, nullptr);
    physics_component* orphan_body = fx.client_scene.get_component<physics_component>(orphan);
    ASSERT_NE(orphan_body->body, nullptr);
    EXPECT_EQ(orphan_body->body->body_type, physics_body::DYNAMIC);
  }

  TEST_F(replication_tests, owner_leave_despawns_owned_objects) {
    repl_sim fx;
    ASSERT_TRUE(fx.connect());
    const uint16_t client_peer = fx.client_session->local_peer_id();

    scene_object& pawn = add_cube(fx.host_scene, "pawn", { 0.f, 0.f, 0.f });
    network_component& net = fx.host_scene.add_component<network_component>(&pawn);
    net.owner_peer = client_peer;
    net.despawn_on_owner_leave = true;
    scene_object& keeper = add_cube(fx.host_scene, "keeper", { 1.f, 0.f, 0.f });
    network_component& keeper_net = fx.host_scene.add_component<network_component>(&keeper);
    keeper_net.owner_peer = client_peer;
    keeper_net.despawn_on_owner_leave = false;
    fx.step(8);  // register + replicate

    fx.client_session->leave();
    ASSERT_TRUE(fx.step_until([&] { return fx.host_scene.find_object("pawn") == nullptr; }));
    EXPECT_NE(fx.host_scene.find_object("keeper"), nullptr);
  }

  TEST_F(replication_tests, unknown_net_id_batches_are_ignored_not_fatal) {
    repl_sim fx;
    ASSERT_TRUE(fx.connect());
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.network().role == replication_role::REPLICA; }));

    net_transform_batch bogus;
    bogus.host_tick = 999;
    bogus.entries.push_back({ .net_id = 0xDEAD, .position = { 1.f, 2.f, 3.f }, .rotation = {}, .scale = { 1.f, 1.f, 1.f } });
    fx.host_session->broadcast(net_message::TRANSFORM_BATCH, serialize_direct(bogus));
    fx.step(8);

    EXPECT_TRUE(fx.client_session->in_session());
    EXPECT_EQ(fx.client_scene.network().role, replication_role::REPLICA);
  }

  TEST_F(replication_tests, late_join_receives_current_not_initial_state) {
    repl_sim fx;
    scene_object& mover = add_cube(fx.host_scene, "mover", { 0.f, 0.f, 0.f });
    fx.host_scene.add_component<network_component>(&mover);

    /// the world runs before anyone joins
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    fx.step(4);
    fx.host_scene.get_component<transform>(&mover)->local_position = { 42.f, 0.f, 0.f };
    fx.step(4);

    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("mover") != nullptr; }));
    EXPECT_FLOAT_EQ(client_x(fx, "mover"), 42.f);
  }

  TEST_F(replication_tests, codec_vector_of_trivial_round_trips) {
    net_transform_batch batch;
    batch.host_tick = 7;
    for (uint32_t i = 0; i < 5; ++i) {
      batch.entries.push_back({ .net_id = i + 1,
                                .position = { static_cast<float>(i), 2.f, 3.f },
                                .rotation = glm::quat(1.f, 0.f, 0.f, 0.f),
                                .scale = { 1.f, 1.f, 1.f } });
    }
    auto [decoded, consumed] = deserialize_direct<net_transform_batch>(serialize_direct(batch));
    EXPECT_EQ(decoded.host_tick, 7u);
    ASSERT_EQ(decoded.entries.size(), 5u);
    EXPECT_EQ(decoded.entries[3].net_id, 4u);
    EXPECT_FLOAT_EQ(decoded.entries[4].position.x, 4.f);

    const net_transform_batch empty;
    auto [decoded_empty, consumed_empty] = deserialize_direct<net_transform_batch>(serialize_direct(empty));
    EXPECT_TRUE(decoded_empty.entries.empty());
  }

  /// ---------------------------------------------------- shaped-link battery

  TEST_F(replication_tests, join_completes_over_200ms_jittered_link) {
    /// default mesh/session timers: the shaped handshake needs the real headroom
    repl_sim fx({}, peer_mesh_config{}, network_session::session_config{});
    add_cube(fx.host_scene, "floor", { 0.f, -1.f, 0.f });
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));

    const ostd::vector<link_record> dialed = fx.client_session->links();
    ASSERT_EQ(dialed.size(), 1u);
    const link_profile shaped{ .latency = microseconds{ 200'000 }, .jitter = microseconds{ 40'000 }, .seed = 11 };
    fx.sim.fabric.set_profile(dialed[0].connection_id, shaped, shaped);

    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("floor") != nullptr; }, 512, microseconds{ 10'000 }));
    EXPECT_EQ(fx.client_scene.network().role, replication_role::REPLICA);
  }

  TEST_F(replication_tests, interpolation_stays_smooth_at_150ms_rtt) {
    repl_sim fx;
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    const ostd::vector<link_record> dialed = fx.client_session->links();
    ASSERT_EQ(dialed.size(), 1u);
    const link_profile shaped{ .latency = microseconds{ 75'000 }, .seed = 3 };
    fx.sim.fabric.set_profile(dialed[0].connection_id, shaped, shaped);
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }, 512, microseconds{ 10'000 }));

    scene_object& mover = add_cube(fx.host_scene, "mover", { 0.f, 0.f, 0.f });
    ASSERT_TRUE(fx.host_repl->spawn_object(mover.id));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("mover") != nullptr; }, 512, microseconds{ 10'000 }));

    transform* host_trs = fx.host_scene.get_component<transform>(&mover);
    ostd::vector<float> observed;
    for (size_t i = 0; i < 200; ++i) {
      host_trs->local_position.x = static_cast<float>(fx.host_repl->host_tick());
      fx.step(1, microseconds{ 10'000 });
      observed.push_back(client_x(fx, "mover"));
    }
    /// no pose snaps beyond epsilon between frames once the buffer is warm
    for (size_t i = 120; i < observed.size(); ++i) {
      EXPECT_LT(glm::abs(observed[i] - observed[i - 1]), 0.8f);
    }
    EXPECT_GT(observed.back(), observed[120]);
  }

  TEST_F(replication_tests, snapshot_cadence_survives_bandwidth_throttle) {
    repl_sim fx;
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    const ostd::vector<link_record> dialed = fx.client_session->links();
    ASSERT_EQ(dialed.size(), 1u);
    const link_profile throttled{ .bandwidth = 512'000, .seed = 5 };  // 512 kbps
    fx.sim.fabric.set_profile(dialed[0].connection_id, throttled, throttled);
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }, 512, microseconds{ 10'000 }));

    ostd::vector<scene_object*> movers;
    for (int i = 0; i < 20; ++i) {
      scene_object& mover = add_cube(fx.host_scene, "mover-" + std::to_string(i), { 0.f, static_cast<float>(i), 0.f });
      ASSERT_TRUE(fx.host_repl->spawn_object(mover.id));
      movers.push_back(&mover);
    }

    for (size_t i = 0; i < 400; ++i) {
      for (scene_object* mover : movers) {
        fx.host_scene.get_component<transform>(mover)->local_position.x = static_cast<float>(fx.host_repl->host_tick());
      }
      fx.step(1, microseconds{ 10'000 });
    }

    /// batches thin under the throttle; nothing corrupts, the session survives,
    ///  and the replica keeps making progress
    EXPECT_TRUE(fx.client_session->in_session());
    EXPECT_TRUE(fx.host_session->in_session());
    EXPECT_EQ(fx.client_scene.get_object_count(), fx.host_scene.get_object_count());
    EXPECT_GT(client_x(fx, "mover-0"), 1.f);
  }

  /// ---------------------------------------------------- tcp end to end

  TEST_F(replication_tests, two_peers_localhost_spawn_and_move) {
    scene host_scene{ "host-world" };
    scene client_scene{ "client-world" };
    socket_net_instance host_side("repl-host");
    socket_net_instance client_side("repl-client");
    ASSERT_TRUE(host_side.start());
    ASSERT_TRUE(client_side.start());

    /// real sockets under real suite load: default join timeout, not the sim-scale one
    network_session& host = static_cast<network_session&>(
      host_side.mesh.set_primary(make_scope<network_session>(network_session::session_config{ .display_name = "editor-a" }), 1));
    network_session& client = static_cast<network_session&>(
      client_side.mesh.set_primary(make_scope<network_session>(network_session::session_config{ .display_name = "editor-b" }), 2));
    replication host_repl(host, [&] { return &host_scene; });
    replication client_repl(client, [&] { return &client_scene; });
    host.set_observer([&](session_event ev, uint16_t arg) { host_repl.on_session_event(ev, arg); });
    client.set_observer([&](session_event ev, uint16_t arg) { client_repl.on_session_event(ev, arg); });

    const uint16_t port = next_test_port();
    ASSERT_TRUE(host.host(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));
    ASSERT_TRUE(client.join(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));

    const auto pump = [&] {
      host_side.pump();
      client_side.pump();
      host_repl.tick(host_side.now());
      client_repl.tick(client_side.now());
    };
    /// generous: localhost sockets share the machine with the rest of the suite
    const auto pump_until_true = [&](auto&& pred, int deadline_ms = 15'000) {
      const auto start = std::chrono::steady_clock::now();
      while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(deadline_ms)) {
        pump();
        if (pred()) {
          return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return pred();
    };

    ASSERT_TRUE(pump_until_true([&] { return client.in_session(); }));

    scene_object& crate = add_cube(host_scene, "crate", { 1.f, 0.f, 0.f });
    ASSERT_TRUE(host_repl.spawn_object(crate.id));
    ASSERT_TRUE(pump_until_true([&] { return client_scene.find_object("crate") != nullptr; }));

    host_scene.get_component<transform>(&crate)->local_position.x = 25.f;
    ASSERT_TRUE(pump_until_true([&] {
      scene_object* replica = client_scene.find_object("crate");
      return replica != nullptr && client_scene.get_component<transform>(replica)->local_position.x > 20.f;
    }));

    host.set_observer(nullptr);
    client.set_observer(nullptr);
  }

}  // namespace other
