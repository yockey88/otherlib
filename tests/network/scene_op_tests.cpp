/**
 * \file network/scene_op_tests.cpp
 *  M5: validated shared world mutation — request/validate/apply/journal/broadcast
 *  on the sim mesh; one end-to-end row proves intent -> state over tcp.
 **/
#include <algorithm>

#include <gtest/gtest.h>

#include "object/transform.hpp"
#include "scene/scene.hpp"

#include "network/session/net_messages.hpp"
#include "network/session/network_session.hpp"
#include "network/session/replication.hpp"
#include "network/session/scene_ops.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "network/mesh_sim_fixture.hpp"
#include "network/socket_mesh_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class scene_op_tests : public other_test {
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

    struct recorded_op {
      uint16_t actor = 0;
      std::string name;
      natural_t subject = 0;
      ostd::vector<uint8_t> payload;
    };

    struct recorded_rejection {
      std::string name;
      uint16_t reason = 0;
    };

    op_channel::applied_handler record_applied(ostd::vector<recorded_op>& sink) {
      return [&sink](const scene_op& op) {
        sink.push_back({ op.actor, std::string(op.name.begin(), op.name.end()), op.subject, op.payload });
      };
    }

    op_channel::rejected_handler record_rejected(ostd::vector<recorded_rejection>& sink) {
      return [&sink](std::string_view name, uint16_t reason) {
        sink.push_back({ std::string(name), reason });
      };
    }

    /// host + up to two client seats with op channels; no scenes — the channel is
    ///  scene-free by design (gameplay handlers own mutation)
    struct ops_sim {
      mesh_sim_fixture sim;
      network_session* host_session = nullptr;
      network_session* a_session = nullptr;
      network_session* b_session = nullptr;
      scope<op_channel> host_ops;
      scope<op_channel> a_ops;
      scope<op_channel> b_ops;
      uint64_t tick = 0;
      uint64_t endpoint = 0;

      explicit ops_sim(size_t clients = 1, size_t journal_cap = 4096)
          : sim(0, 1, fast_cfg()) {
        const auto ticks = [this] { return tick; };
        host_session = &spawn(1, "host");
        host_ops = make_scope<op_channel>(*host_session, ticks, journal_cap);
        a_session = &spawn(2, "a");
        a_ops = make_scope<op_channel>(*a_session, ticks, journal_cap);
        if (clients > 1) {
          b_session = &spawn(3, "b");
          b_ops = make_scope<op_channel>(*b_session, ticks, journal_cap);
        }

        endpoint = sim.next_endpoint++;
        sim.fabric.configure_endpoint(endpoint, false);
      }

      network_session& spawn(node_id node, std::string name) {
        return static_cast<network_session&>(sim.mesh().spawn_actor(make_scope<network_session>(fast_scfg(std::move(name))), node));
      }

      template <typename Pred>
      bool step_until(Pred&& done, size_t max_ticks = 256) {
        for (size_t i = 0; i < max_ticks; ++i) {
          if (done()) {
            return true;
          }
          sim.step(microseconds{ 5'000 });
          tick++;
        }
        return done();
      }

      bool connect() {
        if (!host_session->host(net_address::memory_endpoint(endpoint)) ||
            !a_session->join(net_address::memory_endpoint(endpoint))) {
          return false;
        }
        if (b_session != nullptr && !b_session->join(net_address::memory_endpoint(endpoint))) {
          return false;
        }
        return step_until([&] {
          return a_session->in_session() && (b_session == nullptr || b_session->in_session());
        });
      }
    };

    std::string op_name(const scene_op& op) {
      return std::string(op.name.begin(), op.name.end());
    }

  }  // namespace

  TEST_F(scene_op_tests, op_request_validate_apply_broadcast_round_trip) {
    ostd::vector<recorded_op> host_applied, client_applied;
    size_t validated = 0;
    ops_sim fx;
    fx.host_ops->set_validator([&](uint16_t peer, const scene_op& op) -> op_result {
      validated++;
      EXPECT_EQ(op_name(op), "construct.place-part");
      return {};
    });
    fx.host_ops->set_applied_handler(record_applied(host_applied));
    fx.a_ops->set_applied_handler(record_applied(client_applied));
    ASSERT_TRUE(fx.connect());

    const ostd::vector<uint8_t> payload{ 9, 8, 7 };
    ASSERT_TRUE(fx.a_ops->request("construct.place-part", 42, payload));
    ASSERT_TRUE(fx.step_until([&] { return !client_applied.empty(); }));

    EXPECT_EQ(validated, 1u);
    ASSERT_EQ(fx.host_ops->journal().size(), 1u);
    const scene_op& journaled = fx.host_ops->journal()[0];
    EXPECT_EQ(journaled.op_id, 1u);
    EXPECT_EQ(journaled.actor, fx.a_session->local_peer_id());
    EXPECT_EQ(journaled.subject, 42u);
    EXPECT_EQ(op_name(journaled), "construct.place-part");

    /// one dispatch path: the host ran its own applied handler too
    ASSERT_EQ(host_applied.size(), 1u);
    ASSERT_EQ(client_applied.size(), 1u);
    EXPECT_EQ(client_applied[0].actor, fx.a_session->local_peer_id());
    EXPECT_EQ(client_applied[0].payload, payload);
  }

  TEST_F(scene_op_tests, rejected_op_reaches_only_requester_and_mutates_nothing) {
    ostd::vector<recorded_op> a_applied, b_applied;
    ostd::vector<recorded_rejection> a_rejected, b_rejected;
    ops_sim fx(2);
    fx.host_ops->set_validator([](uint16_t, const scene_op&) -> op_result {
      return { .accepted = false, .reason = 7 };
    });
    fx.a_ops->set_applied_handler(record_applied(a_applied));
    fx.a_ops->set_rejected_handler(record_rejected(a_rejected));
    fx.b_ops->set_applied_handler(record_applied(b_applied));
    fx.b_ops->set_rejected_handler(record_rejected(b_rejected));
    ASSERT_TRUE(fx.connect());

    ASSERT_TRUE(fx.a_ops->request("construct.place-part", 0, {}));
    ASSERT_TRUE(fx.step_until([&] { return !a_rejected.empty(); }));

    EXPECT_EQ(a_rejected[0].name, "construct.place-part");
    EXPECT_EQ(a_rejected[0].reason, 7);
    EXPECT_TRUE(a_applied.empty());
    EXPECT_TRUE(b_applied.empty());
    EXPECT_TRUE(b_rejected.empty());
    EXPECT_TRUE(fx.host_ops->journal().empty());
  }

  TEST_F(scene_op_tests, host_initiated_ops_share_the_write_path) {
    ostd::vector<recorded_op> client_applied;
    size_t validated = 0;
    ops_sim fx;
    fx.host_ops->set_validator([&](uint16_t, const scene_op&) -> op_result {
      validated++;
      return {};
    });
    fx.a_ops->set_applied_handler(record_applied(client_applied));
    ASSERT_TRUE(fx.connect());

    ASSERT_TRUE(fx.host_ops->request("world.weather", 0, {}));
    ASSERT_TRUE(fx.step_until([&] { return !client_applied.empty(); }));

    EXPECT_EQ(validated, 0u);  // the request hop is skipped, not the journal
    ASSERT_EQ(fx.host_ops->journal().size(), 1u);
    EXPECT_EQ(fx.host_ops->journal()[0].actor, 0);
    EXPECT_EQ(client_applied[0].actor, 0);
  }

  TEST_F(scene_op_tests, journal_ring_caps_and_orders_by_op_id) {
    ops_sim fx(1, /*journal_cap=*/8);
    ASSERT_TRUE(fx.connect());

    for (int i = 0; i < 20; ++i) {
      ASSERT_TRUE(fx.host_ops->request("op." + std::to_string(i), 0, {}));
    }

    const std::span<const scene_op> journal = fx.host_ops->journal();
    ASSERT_EQ(journal.size(), 8u);
    EXPECT_EQ(journal.front().op_id, 13u);
    EXPECT_EQ(journal.back().op_id, 20u);
    EXPECT_TRUE(std::ranges::is_sorted(journal, {}, &scene_op::op_id));
  }

  TEST_F(scene_op_tests, op_ids_and_ticks_are_monotonic_across_a_session) {
    ops_sim fx;
    ASSERT_TRUE(fx.connect());

    for (int round = 0; round < 5; ++round) {
      ASSERT_TRUE(fx.host_ops->request("host.op", 0, {}));
      ASSERT_TRUE(fx.a_ops->request("client.op", 0, {}));
      fx.step_until([&] { return fx.host_ops->journal().size() == static_cast<size_t>(round * 2 + 2); });
    }

    const std::span<const scene_op> journal = fx.host_ops->journal();
    ASSERT_EQ(journal.size(), 10u);
    for (size_t i = 1; i < journal.size(); ++i) {
      EXPECT_EQ(journal[i].op_id, journal[i - 1].op_id + 1);
      EXPECT_GE(journal[i].tick, journal[i - 1].tick);
    }
  }

  TEST_F(scene_op_tests, client_applied_handler_fires_without_state_authority) {
    ostd::vector<recorded_op> client_applied;
    ops_sim fx;
    fx.a_ops->set_applied_handler(record_applied(client_applied));
    ASSERT_TRUE(fx.connect());

    ASSERT_TRUE(fx.host_ops->request("fx.explosion", 5, {}));
    ASSERT_TRUE(fx.step_until([&] { return !client_applied.empty(); }));

    /// presentation only: the client dispatched but journaled nothing — the
    ///  authoritative record lives with the authority
    EXPECT_TRUE(fx.a_ops->journal().empty());
    EXPECT_EQ(fx.host_ops->journal().size(), 1u);
  }

  TEST_F(scene_op_tests, session_churn_under_continuous_op_traffic) {
    ostd::vector<recorded_op> client_applied;
    ops_sim fx;
    fx.a_ops->set_applied_handler(record_applied(client_applied));
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));

    /// 20 join/leave cycles with op traffic inside each membership window
    for (int cycle = 0; cycle < 20; ++cycle) {
      ASSERT_TRUE(fx.a_session->join(net_address::memory_endpoint(fx.endpoint))) << "cycle " << cycle;
      ASSERT_TRUE(fx.step_until([&] { return fx.a_session->in_session(); })) << "cycle " << cycle;

      const size_t seen = client_applied.size();
      ASSERT_TRUE(fx.a_ops->request("cycle.op", static_cast<natural_t>(cycle), {}));
      ASSERT_TRUE(fx.host_ops->request("host.op", 0, {}));
      ASSERT_TRUE(fx.step_until([&] { return client_applied.size() >= seen + 2; })) << "cycle " << cycle;

      fx.a_session->leave();
      /// the host processes the notice a few ticks after the client's local leave
      ASSERT_TRUE(fx.step_until([&] { return fx.host_session->peers().size() == 1; })) << "cycle " << cycle;
    }

    EXPECT_TRUE(fx.host_session->in_session());
    EXPECT_EQ(fx.host_session->peers().size(), 1u);
    const std::span<const scene_op> journal = fx.host_ops->journal();
    EXPECT_EQ(journal.size(), 40u);
    EXPECT_TRUE(std::ranges::is_sorted(journal, {}, &scene_op::op_id));
  }

  TEST_F(scene_op_tests, tcp_op_request_spawns_replicated_object_both_sides) {
    scene host_scene{ "host-world" };
    scene client_scene{ "client-world" };
    socket_net_instance host_side("ops-host");
    socket_net_instance client_side("ops-client");
    ASSERT_TRUE(host_side.start());
    ASSERT_TRUE(client_side.start());

    network_session& host = static_cast<network_session&>(
      host_side.mesh.spawn_actor(make_scope<network_session>(network_session::session_config{ .display_name = "editor-a" }), 1));
    network_session& client = static_cast<network_session&>(
      client_side.mesh.spawn_actor(make_scope<network_session>(network_session::session_config{ .display_name = "editor-b" }), 2));
    replication host_repl(host, [&] { return &host_scene; });
    replication client_repl(client, [&] { return &client_scene; });
    host.set_observer([&](session_event ev, uint16_t arg) { host_repl.on_session_event(ev, arg); });
    client.set_observer([&](session_event ev, uint16_t arg) { client_repl.on_session_event(ev, arg); });
    op_channel host_ops(host, [&] { return host_repl.host_tick(); });
    op_channel client_ops(client, [&] { return client_repl.host_tick(); });

    /// validation and application are one gameplay handler on the host: it mutates
    ///  the world through normal APIs and the mutation replicates via M4
    ostd::vector<recorded_op> client_applied;
    host_ops.set_validator([&](uint16_t, const scene_op& op) -> op_result {
      return { .accepted = op_name(op) == "spawn.crate" };
    });
    host_ops.set_applied_handler([&](const scene_op& op) {
      scene_object& crate = host_scene.create_object("crate");
      host_scene.get_component<transform>(&crate)->local_position = { 4.f, 0.f, 0.f };
      host_repl.spawn_object(crate.id, op.actor);
    });
    client_ops.set_applied_handler(record_applied(client_applied));

    const uint16_t port = next_test_port();
    ASSERT_TRUE(host.host(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));
    ASSERT_TRUE(client.join(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));

    const auto pump = [&] {
      host_side.pump();
      client_side.pump();
      host_repl.tick(host_side.now());
      client_repl.tick(client_side.now());
    };
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
    ASSERT_TRUE(client_ops.request("spawn.crate", 0, {}));
    ASSERT_TRUE(pump_until_true([&] { return client_scene.find_object("crate") != nullptr; }));

    /// intent -> state, both sides: the envelope journaled, the object replicated
    EXPECT_NE(host_scene.find_object("crate"), nullptr);
    ASSERT_EQ(host_ops.journal().size(), 1u);
    EXPECT_EQ(host_ops.journal()[0].actor, client.local_peer_id());
    ASSERT_FALSE(client_applied.empty());

    host.set_observer(nullptr);
    client.set_observer(nullptr);
  }

}  // namespace other
