/**
 * \file network/network_session_tests.cpp
 **/
#include <algorithm>

#include <gtest/gtest.h>

#include "network/mesh_sim_fixture.hpp"
#include "network/session/net_messages.hpp"
#include "network/session/network_session.hpp"
#include "network/session/script_actor.hpp"
#include "network/socket_mesh_fixture.hpp"

#include "driver/environment_registry.hpp"

#include "other_test.hpp"
#include "peer_mesh/peer_mesh.hpp"


namespace other {

  class network_session_tests : public other_test {};

  namespace {

    peer_mesh_config fast_cfg() {
      peer_mesh_config cfg;
      cfg.handshake_timeout = microseconds{ 100'000 };
      cfg.keepalive_idle = microseconds{ 50'000 };
      cfg.link_timeout = microseconds{ 200'000 };
      return cfg;
    }

    network_session::session_config fast_session_cfg(std::string display_name = "peer") {
      network_session::session_config cfg;
      cfg.join_timeout = microseconds{ 50'000 };
      cfg.display_name = std::move(display_name);
      return cfg;
    }

    struct probe {
      ostd::vector<std::pair<session_event, uint16_t>> log;

      network_session::session_observer handler() {
        return [this](session_event ev, uint16_t arg) {
          log.push_back({ ev, arg });
        };
      }
      size_t count(session_event ev) const {
        return std::ranges::count(log, ev, &std::pair<session_event, uint16_t>::first);
      }
      opt<uint16_t> arg_of(session_event ev) const {
        auto itr = std::ranges::find(log, ev, &std::pair<session_event, uint16_t>::first);
        return itr != log.end() ? opt<uint16_t>{ itr->second } : std::nullopt;
      }
    };

    struct recorded_game_event {
      uint16_t sender = 0;
      std::string name;
      ostd::vector<uint8_t> payload;
    };

    network_session::game_event_handler record_into(ostd::vector<recorded_game_event>& sink) {
      return [&sink](uint16_t sender, std::string_view name, std::span<const uint8_t> payload) {
        sink.push_back({ sender, std::string(name), ostd::vector<uint8_t>(payload.begin(), payload.end()) });
      };
    }

    network_session& spawn_session(mesh_sim_fixture& sim, node_id node, const network_session::session_config& cfg) {
      return static_cast<network_session&>(sim.spawn(make_scope<network_session>(cfg), node));
    }

    uint64_t open_endpoint(mesh_sim_fixture& sim, bool datagram = false) {
      const uint64_t endpoint = sim.next_endpoint++;
      sim.fabric.configure_endpoint(endpoint, datagram);
      return endpoint;
    }

    const session_member* member_of(const network_session& session, uint16_t peer_id) {
      auto itr = std::ranges::find(session.peers(), peer_id, &session_member::peer_id);
      return itr != session.peers().end() ? &*itr : nullptr;
    }

  }  // namespace

  TEST_F(network_session_tests, join_completes_and_assigns_peer_id_and_roster) {
    probe host_probe, client_probe;  // outlives the mesh: teardown still notifies
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg("alpha"));
    network_session& client = spawn_session(sim, 2, fast_session_cfg("bravo"));
    host.set_observer(host_probe.handler());
    client.set_observer(client_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    EXPECT_TRUE(host.is_host());
    EXPECT_EQ(host.local_peer_id(), 0);
    ASSERT_TRUE(client.join(net_address::memory_endpoint(endpoint)));

    ASSERT_TRUE(sim.step_until([&] { return client.in_session(); }));
    EXPECT_EQ(client.local_peer_id(), 1);
    EXPECT_FALSE(client.is_host());

    ASSERT_EQ(host.peers().size(), 2u);
    ASSERT_EQ(client.peers().size(), 2u);
    const session_member* host_seen_by_client = member_of(client, 0);
    ASSERT_NE(host_seen_by_client, nullptr);
    EXPECT_EQ(host_seen_by_client->node, 1u);
    EXPECT_EQ(host_seen_by_client->name, "alpha");
    const session_member* client_seen_by_host = member_of(host, 1);
    ASSERT_NE(client_seen_by_host, nullptr);
    EXPECT_EQ(client_seen_by_host->node, 2u);
    EXPECT_EQ(client_seen_by_host->name, "bravo");

    EXPECT_EQ(host_probe.count(session_event::STARTED), 1u);
    EXPECT_EQ(host_probe.count(session_event::PEER_JOINED), 1u);
    EXPECT_EQ(client_probe.count(session_event::STARTED), 1u);
    EXPECT_EQ(client_probe.arg_of(session_event::STARTED), opt<uint16_t>{ 1 });
  }

  TEST_F(network_session_tests, join_rejected_when_full) {
    probe second_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session::session_config host_cfg = fast_session_cfg();
    host_cfg.max_peers = 2;
    network_session& host = spawn_session(sim, 1, host_cfg);
    network_session& first = spawn_session(sim, 2, fast_session_cfg());
    network_session& second = spawn_session(sim, 3, fast_session_cfg());
    second.set_observer(second_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(first.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return first.in_session(); }));

    ASSERT_TRUE(second.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return second_probe.count(session_event::ENDED) > 0; }));
    EXPECT_FALSE(second.in_session());
    EXPECT_EQ(second_probe.arg_of(session_event::ENDED),
              opt<uint16_t>{ static_cast<uint16_t>(net_reject_reason::FULL) });
    EXPECT_EQ(host.peers().size(), 2u);
  }

  TEST_F(network_session_tests, join_rejected_on_datagram_link) {
    probe client_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    network_session& client = spawn_session(sim, 2, fast_session_cfg());
    client.set_observer(client_probe.handler());

    const uint64_t endpoint = open_endpoint(sim, true);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(client.join(net_address::memory_endpoint(endpoint)));

    ASSERT_TRUE(sim.step_until([&] { return client_probe.count(session_event::ENDED) > 0; }));
    EXPECT_FALSE(client.in_session());
    EXPECT_EQ(client_probe.arg_of(session_event::ENDED),
              opt<uint16_t>{ static_cast<uint16_t>(net_reject_reason::UNRELIABLE_LINK) });
    EXPECT_EQ(host.peers().size(), 1u);
  }

  TEST_F(network_session_tests, reject_reaches_client_before_close) {
    probe client_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session::session_config host_cfg = fast_session_cfg();
    host_cfg.max_peers = 1;  // nobody gets in
    network_session& host = spawn_session(sim, 1, host_cfg);
    network_session& client = spawn_session(sim, 2, fast_session_cfg());
    client.set_observer(client_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(client.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return client_probe.count(session_event::ENDED) > 0; }));

    /// the REJECT frame must outrun the close: the recorded reason is the policy
    ///  reason, not a link teardown cause — and the session ends exactly once
    EXPECT_EQ(client_probe.arg_of(session_event::ENDED),
              opt<uint16_t>{ static_cast<uint16_t>(net_reject_reason::FULL) });
    sim.step(microseconds{ 1000 }, 32);
    EXPECT_EQ(client_probe.count(session_event::ENDED), 1u);
  }

  TEST_F(network_session_tests, join_validator_rejects_with_custom_reason) {
    constexpr uint16_t kNoTokenReason = 0x0100;  // an application's own reject page
    probe refused_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    host.set_join_validator([kNoTokenReason](const link_record&, const net_join_request& request) -> opt<uint16_t> {
      if (request.client_flags != 7) {
        return kNoTokenReason;
      }
      return std::nullopt;
    });

    network_session::session_config tokenless = fast_session_cfg();
    network_session::session_config tokened = fast_session_cfg();
    tokened.client_flags = 7;
    network_session& refused = spawn_session(sim, 2, tokenless);
    network_session& admitted = spawn_session(sim, 3, tokened);
    refused.set_observer(refused_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(refused.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return refused_probe.count(session_event::ENDED) > 0; }));
    EXPECT_EQ(refused_probe.arg_of(session_event::ENDED), opt<uint16_t>{ kNoTokenReason });

    ASSERT_TRUE(admitted.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return admitted.in_session(); }));
  }

  TEST_F(network_session_tests, join_timeout_drops_silent_link_member) {
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    sim.spawn(make_scope<test_actor>(), 9);

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));

    /// a mesh-level dial that never speaks the session protocol
    ASSERT_NE(sim.base_actor(9).open_link(net_address::memory_endpoint(endpoint)), 0u);
    ASSERT_TRUE(sim.step_until([&] { return sim.mesh().link_between(1, 9) != nullptr; }));

    ASSERT_TRUE(sim.step_until([&] { return sim.mesh().link_between(1, 9) == nullptr; }, 128));
    EXPECT_EQ(host.peers().size(), 1u);
  }

  TEST_F(network_session_tests, peer_joined_and_left_notices_reach_all_members) {
    probe alpha_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    network_session& alpha = spawn_session(sim, 2, fast_session_cfg("alpha"));
    network_session& beta = spawn_session(sim, 3, fast_session_cfg("beta"));
    alpha.set_observer(alpha_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(alpha.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return alpha.in_session(); }));

    ASSERT_TRUE(beta.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return alpha_probe.count(session_event::PEER_JOINED) > 0; }));
    const uint16_t beta_peer = *alpha_probe.arg_of(session_event::PEER_JOINED);
    EXPECT_EQ(alpha.peers().size(), 3u);
    const session_member* beta_seen_by_alpha = member_of(alpha, beta_peer);
    ASSERT_NE(beta_seen_by_alpha, nullptr);
    EXPECT_EQ(beta_seen_by_alpha->name, "beta");

    beta.leave();
    ASSERT_TRUE(sim.step_until([&] { return alpha_probe.count(session_event::PEER_LEFT) > 0; }));
    EXPECT_EQ(alpha_probe.arg_of(session_event::PEER_LEFT), opt<uint16_t>{ beta_peer });
    EXPECT_EQ(alpha.peers().size(), 2u);
    EXPECT_EQ(host.peers().size(), 2u);
  }

  TEST_F(network_session_tests, link_down_is_session_leave_on_both_sides) {
    probe alpha_probe, beta_probe;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    network_session& alpha = spawn_session(sim, 2, fast_session_cfg());
    network_session& beta = spawn_session(sim, 3, fast_session_cfg());
    alpha.set_observer(alpha_probe.handler());
    beta.set_observer(beta_probe.handler());

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(alpha.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(beta.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return alpha.in_session() && beta.in_session(); }));
    const uint16_t beta_peer = beta.local_peer_id();

    /// kill the fabric channel under beta's host link
    const link_record* beta_link = sim.mesh().link_between(3, 1);
    ASSERT_NE(beta_link, nullptr);
    sim.fabric.set_mute(beta_link->connection_id, true, true);

    ASSERT_TRUE(sim.step_until([&] { return beta_probe.count(session_event::ENDED) > 0; }, 512));
    EXPECT_FALSE(beta.in_session());

    ASSERT_TRUE(sim.step_until([&] { return alpha_probe.count(session_event::PEER_LEFT) > 0; }, 512));
    EXPECT_EQ(alpha_probe.arg_of(session_event::PEER_LEFT), opt<uint16_t>{ beta_peer });
    EXPECT_TRUE(alpha.in_session());
    EXPECT_EQ(host.peers().size(), 2u);
  }

  TEST_F(network_session_tests, game_event_round_trips_client_to_host_and_back) {
    ostd::vector<recorded_game_event> host_seen, client_seen;
    mesh_sim_fixture sim(0, 1, fast_cfg());
    network_session& host = spawn_session(sim, 1, fast_session_cfg());
    network_session& client = spawn_session(sim, 2, fast_session_cfg());
    host.set_game_event_handler(record_into(host_seen));
    client.set_game_event_handler(record_into(client_seen));

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(client.join(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(sim.step_until([&] { return client.in_session(); }));

    const ostd::vector<uint8_t> ping{ 1, 2, 3 };
    ASSERT_TRUE(client.send_game_event("ping", ping));
    ASSERT_TRUE(sim.step_until([&] { return !host_seen.empty(); }));
    EXPECT_EQ(host_seen[0].sender, client.local_peer_id());
    EXPECT_EQ(host_seen[0].name, "ping");
    EXPECT_EQ(host_seen[0].payload, ping);

    const ostd::vector<uint8_t> pong{ 4, 5 };
    ASSERT_TRUE(host.broadcast_game_event("pong", pong));
    ASSERT_TRUE(sim.step_until([&] { return !client_seen.empty(); }));
    EXPECT_EQ(client_seen[0].sender, 0);
    EXPECT_EQ(client_seen[0].name, "pong");
    EXPECT_EQ(client_seen[0].payload, pong);
  }

  TEST_F(network_session_tests, high_latency_join_still_completes) {
    /// default timers: the shaped link needs headroom the fast profile lacks
    mesh_sim_fixture sim(0, 1);
    network_session& host = spawn_session(sim, 1, {});
    network_session& client = spawn_session(sim, 2, {});

    const uint64_t endpoint = open_endpoint(sim);
    ASSERT_TRUE(host.host(net_address::memory_endpoint(endpoint)));
    ASSERT_TRUE(client.join(net_address::memory_endpoint(endpoint)));

    /// the dialed link exists pre-handshake but has no remote yet — fetch by seat
    const ostd::vector<link_record> dialed = client.links();
    ASSERT_EQ(dialed.size(), 1u);
    const link_profile shaped{ .latency = microseconds{ 200'000 }, .jitter = microseconds{ 40'000 }, .seed = 7 };
    sim.fabric.set_profile(dialed[0].connection_id, shaped, shaped);

    ASSERT_TRUE(sim.step_until([&] { return client.in_session(); }, 400, microseconds{ 10'000 }));
    EXPECT_EQ(host.peers().size(), 2u);
  }

  TEST_F(network_session_tests, session_apis_fail_softly_when_networking_disabled) {
    /// force-disabled networking never spawns the session on a mesh — every API
    ///  is a warn + false from this state, and none may crash
    network_session session;
    EXPECT_FALSE(session.host(net_address::memory_endpoint(1)));
    EXPECT_FALSE(session.join(net_address::memory_endpoint(1)));
    EXPECT_FALSE(session.send(0, net_message::GAME_EVENT, {}));
    EXPECT_FALSE(session.broadcast(net_message::GAME_EVENT, {}));
    EXPECT_FALSE(session.send_game_event("ping", {}));
    EXPECT_FALSE(session.broadcast_game_event("pong", {}));
    session.leave();
    EXPECT_FALSE(session.in_session());
    EXPECT_TRUE(session.peers().empty());
  }

  namespace {

    class parrot_actor final : public peer_actor {
     public:
      std::string_view name() const override { return "parrot"; }
      void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override {
        send(src, net_id, payload);
      }
    };

    /// plugin factories arena-allocate: the installed scope frees through the arena
    void* parrot_factory(void*) {
      return arena_allocator<parrot_actor>{}.allocate();
    }

  }  // namespace

  TEST_F(network_session_tests, custom_actor_name_resolves_through_interface_registry) {
    /// the Mode-4/analyzer seam end to end: a manifest-installed actor resolves by
    ///  name with zero engine edits — exactly what a plugin DLL ships
    environment_registry registry(interface_scope::DRIVER);
    session_actor_source source;
    registry.register_interface<peer_actor>(
      std::function<natural_t(scope<peer_actor>)>([&](scope<peer_actor> actor) { return source.provide(std::move(actor)); }),
      [&](natural_t id) { source.revoke(id); },
      no_args(),
      interface_cardinality::MULTIPLE);

    const plugin_manifest manifest{
      .interface_hash = peer_actor::kInterfaceHash,
      .factory_function = &parrot_factory,
      .class_name = "parrot",
      .plugin_instance_name = "test-parrot",
      .parameters = {},
    };
    ASSERT_NE(registry.install_from_manifest("test-plugin", manifest), 0u);

    EXPECT_EQ(source.take("no-such-actor").actor, nullptr);
    session_actor_source::taken custom = source.take("parrot");
    ASSERT_NE(custom.actor, nullptr);
    EXPECT_NE(custom.provider_id, 0u);

    mesh_sim_fixture sim(1);
    sim.mesh().add_secondary(std::move(custom.actor), 7);
    sim.link(1, 7);
    ASSERT_TRUE(sim.step_until([&] { return sim.mesh().link_between(1, 7) != nullptr; }));

    const ostd::vector<uint8_t> payload{ 0xAB, 0xCD };
    ASSERT_TRUE(sim.base_actor(1).send(7, 100, payload));
    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).frames.empty(); }));
    EXPECT_EQ(sim.actor(1).frames[0].net_id, 100);
    EXPECT_EQ(sim.actor(1).frames[0].payload, payload);
  }

  TEST_F(network_session_tests, two_editors_worth_of_stack_join_over_localhost) {
    ostd::vector<recorded_game_event> host_seen;
    socket_net_instance host_side("session-host");
    socket_net_instance client_side("session-client");
    ASSERT_TRUE(host_side.start());
    ASSERT_TRUE(client_side.start());

    network_session& host = static_cast<network_session&>(
      host_side.mesh.set_primary(make_scope<network_session>(network_session::session_config{ .display_name = "editor-a" }), 1));
    network_session& client = static_cast<network_session&>(
      client_side.mesh.set_primary(make_scope<network_session>(network_session::session_config{ .display_name = "editor-b" }), 2));
    host.set_game_event_handler(record_into(host_seen));

    const uint16_t port = next_test_port();
    ASSERT_TRUE(host.host(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));
    ASSERT_TRUE(client.join(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })));

    ASSERT_TRUE(pump_until({ &host_side, &client_side }, [&] { return client.in_session() && host.peers().size() == 2; }));
    EXPECT_EQ(client.local_peer_id(), 1);
    const session_member* host_seen_by_client = member_of(client, 0);
    ASSERT_NE(host_seen_by_client, nullptr);
    EXPECT_EQ(host_seen_by_client->name, "editor-a");

    const ostd::vector<uint8_t> ping{ 9, 9 };
    ASSERT_TRUE(client.send_game_event("tcp-ping", ping));
    ASSERT_TRUE(pump_until({ &host_side, &client_side }, [&] { return !host_seen.empty(); }));
    EXPECT_EQ(host_seen[0].sender, 1);
    EXPECT_EQ(host_seen[0].payload, ping);
  }

  TEST_F(network_session_tests, script_actor_shim_forwards_callbacks) {
    mesh_sim_fixture sim(1);

    natural_t frames = 0, ups = 0, ticks = 0;
    ostd::vector<uint8_t> last_payload;
    script_actor::callbacks hooks{
      .frame = [&](const link_record&, node_id src, uint16_t net_id, std::span<const uint8_t> payload) {
        frames++;
        EXPECT_EQ(src, 1u);
        EXPECT_EQ(net_id, 77u);
        last_payload.assign(payload.begin(), payload.end());
      },
      .link_up = [&](const link_record&) { ups++; },
      .link_down = [](const link_record&, link_close_reason) {},
      .ticked = [&](microseconds, double) { ticks++; },
    };
    script_actor& script = static_cast<script_actor&>(
      sim.mesh().add_secondary(make_scope<script_actor>("script:Test.Actor", "Test.Actor", std::move(hooks)), 7));
    EXPECT_EQ(script.managed_type(), "Test.Actor");

    sim.link(1, 7);
    ASSERT_TRUE(sim.step_until([&] { return sim.mesh().link_between(1, 7) != nullptr && ups > 0; }));

    const ostd::vector<uint8_t> payload{ 0x11, 0x22, 0x33 };
    ASSERT_TRUE(sim.base_actor(1).send(7, 77, payload));
    ASSERT_TRUE(sim.step_until([&] { return frames > 0; }));
    EXPECT_EQ(last_payload, payload);
    EXPECT_GT(ticks, 0u);

    /// the seat surface works from the shim side too
    EXPECT_TRUE(script.send(1, 78, payload));
    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).frames.empty(); }));
    EXPECT_EQ(sim.actor(1).frames[0].net_id, 78u);
  }

}  // namespace other
