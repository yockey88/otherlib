/**
 * \file network/peer_mesh_tests.cpp
 **/
#include <algorithm>

#include <gtest/gtest.h>

#include "network/memory/memory_fabric.hpp"
#include "peer_mesh/link_security.hpp"
#include "peer_mesh/mesh_messages.hpp"
#include "peer_mesh/mesh_router.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "network/mesh_sim_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class peer_mesh_tests : public other_test {};

  namespace {

    /// short timers so timeout paths run in a few dozen virtual millis
    peer_mesh_config fast_cfg() {
      peer_mesh_config cfg;
      cfg.handshake_timeout = microseconds{ 100'000 };
      cfg.keepalive_idle = microseconds{ 50'000 };
      cfg.link_timeout = microseconds{ 200'000 };
      return cfg;
    }

    ostd::vector<uint8_t> seq_payload(uint32_t i, size_t size = 8) {
      ostd::vector<uint8_t> payload(size);
      for (size_t b = 0; b < size; ++b) {
        payload[b] = static_cast<uint8_t>((i + b) & 0xFF);
      }
      payload[0] = static_cast<uint8_t>(i & 0xFF);
      payload[1] = static_cast<uint8_t>((i >> 8) & 0xFF);
      return payload;
    }

    bool link_up_both(mesh_sim_fixture& sim, node_id a, node_id b) {
      return sim.mesh().net().link_between(a, b) != nullptr && sim.mesh().net().link_between(b, a) != nullptr;
    }

  }  // namespace

  TEST_F(peer_mesh_tests, link_handshake_completes_and_carries_node_ids) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);

    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    const link_record* ab = sim.mesh().net().link_between(1, 2);
    const link_record* ba = sim.mesh().net().link_between(2, 1);
    ASSERT_NE(ab, nullptr);
    ASSERT_NE(ba, nullptr);
    EXPECT_EQ(ab->remote, 2u);
    EXPECT_EQ(ba->remote, 1u);
    EXPECT_TRUE(ab->caps.reliable);
    EXPECT_TRUE(ab->caps.ordered);

    EXPECT_EQ(sim.actor(1).ups.size(), 1u);
    EXPECT_EQ(sim.actor(2).ups.size(), 1u);
    EXPECT_TRUE(sim.mesh().graph().contains(1));
    EXPECT_TRUE(sim.mesh().graph().contains(2));
    const ostd::vector<node_id> neighbors = sim.mesh().graph().neighbors(1);
    EXPECT_TRUE(std::ranges::find(neighbors, 2u) != neighbors.end());
  }

  TEST_F(peer_mesh_tests, handshake_rejects_wrong_app_hash) {
    memory_fabric fabric(7);
    fabric_port port_a(fabric);
    fabric_port port_b(fabric);

    peer_mesh_config cfg_a = fast_cfg();
    cfg_a.app_hash = 111;
    peer_mesh_config cfg_b = fast_cfg();
    cfg_b.app_hash = 222;

    peer_mesh mesh_a("a", cfg_a);
    peer_mesh mesh_b("b", cfg_b);
    mesh_a.register_transport(port_a);
    mesh_b.register_transport(port_b);
    test_actor& actor_a = static_cast<test_actor&>(mesh_a.spawn_actor(make_scope<test_actor>(), 1));
    mesh_b.spawn_actor(make_scope<test_actor>(), 2);

    fabric.configure_endpoint(1, false);
    ASSERT_NE(mesh_b.actor(2)->open_listener(net_address::memory_endpoint(1)), 0u);
    ASSERT_NE(actor_a.open_link(net_address::memory_endpoint(1)), 0u);

    microseconds now{ 0 };
    for (int i = 0; i < 20; ++i) {
      now += microseconds{ 1000 };
      mesh_a.tick(now);
      mesh_b.tick(now);
      fabric.tick(now);
    }

    EXPECT_EQ(mesh_a.net().link_count(), 0u);
    EXPECT_EQ(mesh_b.net().link_count(), 0u);
    EXPECT_GE(mesh_a.counters().protocol_errors + mesh_b.counters().protocol_errors, 1u);
    ASSERT_FALSE(actor_a.downs.empty());
    EXPECT_EQ(actor_a.downs.front().reason, link_close_reason::PROTOCOL_ERROR);
  }

  TEST_F(peer_mesh_tests, handshake_timeout_drops_silent_link) {
    mesh_sim_fixture sim(1, 1, fast_cfg());

    /// a listener owned by a port bound to nothing: accepts, never speaks
    fabric_port silent(sim.fabric);
    silent.bind({});
    sim.fabric.configure_endpoint(99, false);
    ASSERT_NE(silent.listen(net_address::memory_endpoint(99)), 0u);

    const natural_t link_id = sim.actor(1).open_link(net_address::memory_endpoint(99));
    ASSERT_NE(link_id, 0u);

    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).downs.empty(); }, 256));
    EXPECT_EQ(sim.actor(1).downs.front().reason, link_close_reason::HANDSHAKE_TIMEOUT);
    EXPECT_EQ(sim.mesh().net().link_count(), 0u);
  }

  TEST_F(peer_mesh_tests, keepalive_measures_rtt_over_shaped_link) {
    mesh_sim_fixture sim(2, 1, fast_cfg());
    const link_profile ten_ms{ .latency = microseconds{ 10'000 } };
    sim.link(1, 2, ten_ms, ten_ms);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    /// idle past keepalive_idle -> ping; pong returns after ~2x latency
    ASSERT_TRUE(sim.step_until([&] {
      const link_record* ab = sim.mesh().net().link_between(1, 2);
      return ab != nullptr && ab->rtt.count() > 0;
    }, 256));

    const link_record* ab = sim.mesh().net().link_between(1, 2);
    ASSERT_NE(ab, nullptr);
    EXPECT_GE(ab->rtt.count(), 18'000);
    EXPECT_LE(ab->rtt.count(), 30'000);

    const peer_record* remote = sim.mesh().graph().record(2);
    ASSERT_NE(remote, nullptr);
    EXPECT_EQ(remote->rtt_estimate.count(), ab->rtt.count());
  }

  TEST_F(peer_mesh_tests, keepalive_timeout_drops_muted_link) {
    mesh_sim_fixture sim(2, 1, fast_cfg());
    const natural_t link_id = sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    const link_record* record = sim.mesh().net().link(link_id);
    ASSERT_NE(record, nullptr);
    sim.fabric.set_mute(record->connection_id, true, true);

    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).downs.empty() && !sim.actor(2).downs.empty(); }, 512));
    EXPECT_EQ(sim.actor(1).downs.front().reason, link_close_reason::KEEPALIVE_TIMEOUT);
    EXPECT_EQ(sim.actor(2).downs.front().reason, link_close_reason::KEEPALIVE_TIMEOUT);
    EXPECT_EQ(sim.mesh().net().link_count(), 0u);
  }

  TEST_F(peer_mesh_tests, two_actors_on_one_mesh_exchange_frames) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    const ostd::vector<uint8_t> payload = seq_payload(42, 32);
    EXPECT_TRUE(sim.actor(1).send(2, 7, payload));
    EXPECT_TRUE(sim.actor(2).send(1, 9, payload));

    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).frames.empty() && !sim.actor(2).frames.empty(); }));

    ASSERT_EQ(sim.actor(2).frames.size(), 1u);
    EXPECT_EQ(sim.actor(2).frames[0].src, 1u);
    EXPECT_EQ(sim.actor(2).frames[0].net_id, 7u);
    EXPECT_EQ(sim.actor(2).frames[0].payload, payload);

    ASSERT_EQ(sim.actor(1).frames.size(), 1u);
    EXPECT_EQ(sim.actor(1).frames[0].src, 2u);
    EXPECT_EQ(sim.actor(1).frames[0].net_id, 9u);
  }

  TEST_F(peer_mesh_tests, frames_deliver_to_the_link_owning_actor) {
    mesh_sim_fixture sim(3);
    const natural_t link12 = sim.link(1, 2);
    sim.link(1, 3);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2) && link_up_both(sim, 1, 3); }));

    EXPECT_TRUE(sim.actor(2).send(1, 5, seq_payload(2)));
    EXPECT_TRUE(sim.actor(3).send(1, 5, seq_payload(3)));
    EXPECT_TRUE(sim.actor(1).send_on_link(link12, 6, seq_payload(1)));

    sim.step(microseconds{ 1000 }, 8);

    ASSERT_EQ(sim.actor(1).frames.size(), 2u);
    ostd::vector<node_id> sources{ sim.actor(1).frames[0].src, sim.actor(1).frames[1].src };
    std::ranges::sort(sources);
    EXPECT_EQ(sources[0], 2u);
    EXPECT_EQ(sources[1], 3u);

    ASSERT_EQ(sim.actor(2).frames.size(), 1u);
    EXPECT_EQ(sim.actor(2).frames[0].net_id, 6u);
    EXPECT_TRUE(sim.actor(3).frames.empty());
  }

  TEST_F(peer_mesh_tests, control_page_sends_are_refused) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    EXPECT_FALSE(sim.actor(1).send(2, static_cast<uint16_t>(mesh_message::LINK_HELLO), seq_payload(1)));
    EXPECT_FALSE(sim.actor(1).send(2, kMeshControlFloor, seq_payload(1)));
    EXPECT_GE(sim.mesh().counters().refused_sends, 2u);
  }

  TEST_F(peer_mesh_tests, rx_filter_observes_rewrites_and_drops) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    natural_t seen = 0;
    sim.mesh().add_rx_filter([&](mesh_frame_view& view) {
      seen++;
      if (view.net_id == 100) {
        view.dropped = true;
        return;
      }
      if (view.net_id == 101) {
        view.scratch.assign(4, 0xAB);
        view.payload = view.scratch;
      }
    });

    EXPECT_TRUE(sim.actor(1).send(2, 100, seq_payload(1)));
    EXPECT_TRUE(sim.actor(1).send(2, 101, seq_payload(2)));
    EXPECT_TRUE(sim.actor(1).send(2, 102, seq_payload(3)));
    sim.step(microseconds{ 1000 }, 8);

    EXPECT_EQ(seen, 3u);
    ASSERT_EQ(sim.actor(2).frames.size(), 2u);
    EXPECT_EQ(sim.actor(2).frames[0].net_id, 101u);
    EXPECT_EQ(sim.actor(2).frames[0].payload, ostd::vector<uint8_t>(4, 0xAB));
    EXPECT_EQ(sim.actor(2).frames[1].net_id, 102u);
  }

  TEST_F(peer_mesh_tests, tx_filter_sees_outbound_frames) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    natural_t outbound = 0;
    sim.mesh().add_tx_filter([&](mesh_frame_view& view) {
      if (!is_mesh_control(view.net_id)) {
        outbound++;
        EXPECT_EQ(view.dst, 2u);
      }
    });

    EXPECT_TRUE(sim.actor(1).send(2, 55, seq_payload(1)));
    sim.step(microseconds{ 1000 }, 4);
    EXPECT_EQ(outbound, 1u);
    EXPECT_EQ(sim.actor(2).frames.size(), 1u);
  }

  TEST_F(peer_mesh_tests, routed_frame_forwards_a_b_c_with_src_preserved) {
    mesh_sim_fixture sim(3);
    const natural_t link12 = sim.link(1, 2);
    const natural_t link23 = sim.link(2, 3);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2) && link_up_both(sim, 2, 3); }));

    auto routes = make_scope<static_route_router>();
    routes->set_route(1, 3, link12);
    routes->set_route(2, 3, link23);
    sim.mesh().set_router(std::move(routes));

    const ostd::vector<uint8_t> payload = seq_payload(77, 24);
    EXPECT_TRUE(sim.actor(1).send(3, 42, payload));
    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(3).frames.empty(); }));

    ASSERT_EQ(sim.actor(3).frames.size(), 1u);
    EXPECT_EQ(sim.actor(3).frames[0].src, 1u);
    EXPECT_EQ(sim.actor(3).frames[0].net_id, 42u);
    EXPECT_EQ(sim.actor(3).frames[0].payload, payload);
    EXPECT_TRUE(sim.actor(2).frames.empty());
  }

  TEST_F(peer_mesh_tests, ttl_zero_drops_at_relay) {
    peer_mesh_config cfg;
    cfg.default_ttl = 0;
    mesh_sim_fixture sim(3, 1, cfg);
    const natural_t link12 = sim.link(1, 2);
    const natural_t link23 = sim.link(2, 3);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2) && link_up_both(sim, 2, 3); }));

    auto routes = make_scope<static_route_router>();
    routes->set_route(1, 3, link12);
    routes->set_route(2, 3, link23);
    sim.mesh().set_router(std::move(routes));

    EXPECT_TRUE(sim.actor(1).send(3, 42, seq_payload(1)));
    sim.step(microseconds{ 1000 }, 8);

    EXPECT_TRUE(sim.actor(3).frames.empty());
    const link_record* relay_in = sim.mesh().net().link_between(2, 1);
    ASSERT_NE(relay_in, nullptr);
    EXPECT_EQ(relay_in->stats.ttl_drops, 1u);
  }

  TEST_F(peer_mesh_tests, unroutable_dst_counted_not_fatal) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    EXPECT_FALSE(sim.actor(1).send(999, 42, seq_payload(1)));
    EXPECT_GE(sim.mesh().counters().unroutable_drops, 1u);
    EXPECT_EQ(sim.mesh().net().link_count(), 2u);  // nothing torn down
  }

  TEST_F(peer_mesh_tests, reliable_shaping_preserves_order_under_jitter) {
    mesh_sim_fixture sim(2, 17);
    const link_profile shaped{ .latency = microseconds{ 10'000 }, .jitter = microseconds{ 5'000 } };
    sim.link(1, 2, shaped, shaped);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    constexpr uint32_t kCount = 50;
    for (uint32_t i = 0; i < kCount; ++i) {
      ASSERT_TRUE(sim.actor(1).send(2, 10, seq_payload(i, 8 + (i % 32))));
    }
    ASSERT_TRUE(sim.step_until([&] { return sim.actor(2).frames.size() == kCount; }, 256));

    for (uint32_t i = 0; i < kCount; ++i) {
      const recorded_frame& frame = sim.actor(2).frames[i];
      const uint32_t tag = frame.payload[0] | (static_cast<uint32_t>(frame.payload[1]) << 8);
      EXPECT_EQ(tag, i);
    }
  }

  TEST_F(peer_mesh_tests, datagram_shaping_is_seeded_and_deterministic) {
    const auto run = [](uint64_t seed) {
      mesh_sim_fixture sim(2, seed);
      /// establish clean, then shape — hellos have no retransmit by design (the
      ///  handshake timeout is the recovery path on lossy real links)
      const natural_t link_id = sim.link(1, 2, {}, {}, /*datagram=*/true);
      EXPECT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }, 256));

      const link_record* shaped = sim.mesh().net().link(link_id);
      EXPECT_NE(shaped, nullptr);
      const link_profile lossy{ .loss = 0.25f, .duplicate = 0.1f };
      sim.fabric.set_profile(shaped->connection_id, lossy, {});

      constexpr uint32_t kCount = 200;
      for (uint32_t i = 0; i < kCount; ++i) {
        sim.actor(1).send(2, 10, seq_payload(i));
        sim.step();
      }
      sim.step(microseconds{ 1000 }, 32);

      const link_record* record = sim.mesh().net().link(link_id);
      EXPECT_NE(record, nullptr);
      const memory_fabric::channel_stats* stats = sim.fabric.stats_of(record->connection_id);
      EXPECT_NE(stats, nullptr);

      ostd::vector<uint8_t> trace;
      for (const recorded_frame& frame : sim.actor(2).frames) {
        trace.insert(trace.end(), frame.payload.begin(), frame.payload.end());
      }
      return std::tuple{ sim.actor(2).frames.size(), stats->lost, stats->duplicated, trace };
    };

    const auto [delivered_a, lost_a, dup_a, trace_a] = run(1234);
    const auto [delivered_b, lost_b, dup_b, trace_b] = run(1234);
    const auto [delivered_c, lost_c, dup_c, trace_c] = run(9999);

    EXPECT_GT(lost_a, 20u);
    EXPECT_GT(delivered_a, 100u);

    /// identical seeds => identical delivery traces (D16)
    EXPECT_EQ(delivered_a, delivered_b);
    EXPECT_EQ(lost_a, lost_b);
    EXPECT_EQ(dup_a, dup_b);
    EXPECT_EQ(trace_a, trace_b);

    /// different seed => (overwhelmingly) different trace
    EXPECT_NE(trace_a, trace_c);
  }

  TEST_F(peer_mesh_tests, bandwidth_bucket_delays_bulk_transfer) {
    mesh_sim_fixture sim(2);
    link_profile throttled;
    throttled.bandwidth = 800'000;  // 100 KB/s
    sim.link(1, 2, throttled, {});
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    const ostd::vector<uint8_t> bulk(100 * 1024, 0x5A);
    ASSERT_TRUE(sim.actor(1).send(2, 10, bulk));

    /// ~1s of transmit time at 100 KB/s; not there at 0.5s, there by 1.5s
    sim.step(microseconds{ 10'000 }, 50);
    EXPECT_TRUE(sim.actor(2).frames.empty());
    sim.step(microseconds{ 10'000 }, 100);
    ASSERT_EQ(sim.actor(2).frames.size(), 1u);
    EXPECT_EQ(sim.actor(2).frames[0].payload.size(), bulk.size());
  }

  TEST_F(peer_mesh_tests, sixteen_actor_chain_relays_a_token) {
    mesh_sim_fixture sim(16);
    ostd::vector<natural_t> chain_links;
    for (size_t i = 1; i < 16; ++i) {
      chain_links.push_back(sim.link(i, i + 1));
    }
    ASSERT_TRUE(sim.step_until([&] {
      for (size_t i = 1; i < 16; ++i) {
        if (!link_up_both(sim, i, i + 1)) {
          return false;
        }
      }
      return true;
    }, 256));

    auto routes = make_scope<static_route_router>();
    for (size_t i = 1; i <= 8; ++i) {
      routes->set_route(i, 9, chain_links[i - 1]);
    }
    sim.mesh().set_router(std::move(routes));

    const ostd::vector<uint8_t> token = seq_payload(0xBEEF, 16);
    EXPECT_TRUE(sim.actor(1).send(9, 42, token));
    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(9).frames.empty(); }, 128));

    EXPECT_EQ(sim.actor(9).frames[0].src, 1u);
    EXPECT_EQ(sim.actor(9).frames[0].payload, token);
    for (size_t i = 2; i <= 8; ++i) {
      EXPECT_TRUE(sim.actor(i).frames.empty());
    }
  }

  TEST_F(peer_mesh_tests, arbitrary_binary_payloads_round_trip_unmodified) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2); }));

    /// bytes that LOOK like frame headers, control ids, and route headers — the
    ///  module must pass them through untouched (D22)
    ostd::vector<ostd::vector<uint8_t>> payloads;
    payloads.push_back({});                                     // empty
    payloads.push_back({ 0x00 });                               // single byte
    {
      ostd::vector<uint8_t> tricky;
      const ostd::vector<uint8_t> fake_frame = write_frame(static_cast<uint16_t>(mesh_message::LINK_BYE), seq_payload(1));
      tricky.insert(tricky.end(), fake_frame.begin(), fake_frame.end());
      route_header fake_route{ .src = 0xDEAD, .dst = 0xBEEF, .ttl = 0 };
      uint8_t route_bytes[kRouteHeaderSize];
      write_route_header(fake_route, route_bytes);
      tricky.insert(tricky.end(), std::begin(route_bytes), std::end(route_bytes));
      payloads.push_back(std::move(tricky));
    }
    {
      ostd::vector<uint8_t> big(64 * 1024);
      uint64_t state = 0x12345678;
      for (uint8_t& b : big) {
        state = state * 6364136223846793005ull + 1442695040888963407ull;
        b = static_cast<uint8_t>(state >> 56);
      }
      payloads.push_back(std::move(big));
    }

    uint16_t tag = 1;
    for (const ostd::vector<uint8_t>& payload : payloads) {
      ASSERT_TRUE(sim.actor(1).send(2, tag++, payload));
    }
    ASSERT_TRUE(sim.step_until([&] { return sim.actor(2).frames.size() == payloads.size(); }, 128));

    for (size_t i = 0; i < payloads.size(); ++i) {
      EXPECT_EQ(sim.actor(2).frames[i].net_id, i + 1);
      EXPECT_EQ(sim.actor(2).frames[i].payload, payloads[i]) << "payload " << i << " modified in transit";
    }
    EXPECT_EQ(sim.mesh().counters().protocol_errors, 0u);
    EXPECT_EQ(sim.mesh().counters().malformed_frames, 0u);
  }

  TEST_F(peer_mesh_tests, second_mesh_is_isolated_from_the_first) {
    memory_fabric fabric(3);
    fabric_port port_a(fabric);
    fabric_port port_b(fabric);

    peer_mesh mesh_a("a");
    peer_mesh mesh_b("b");
    mesh_a.register_transport(port_a);
    mesh_b.register_transport(port_b);
    test_actor& a1 = static_cast<test_actor&>(mesh_a.spawn_actor(make_scope<test_actor>(), 1));
    test_actor& b1 = static_cast<test_actor&>(mesh_b.spawn_actor(make_scope<test_actor>(), 1));

    /// same node ids on both meshes: separate networks, no bleed
    fabric.configure_endpoint(10, false);
    ASSERT_NE(b1.open_listener(net_address::memory_endpoint(10)), 0u);
    ASSERT_NE(a1.open_link(net_address::memory_endpoint(10)), 0u);

    microseconds now{ 0 };
    const auto step_all = [&](size_t ticks) {
      for (size_t i = 0; i < ticks; ++i) {
        now += microseconds{ 1000 };
        mesh_a.tick(now);
        mesh_b.tick(now);
        fabric.tick(now);
      }
    };
    step_all(8);

    /// the cross-mesh link is up: a's remote is b's node id, learned over the wire
    ASSERT_EQ(mesh_a.net().link_count(), 1u);
    ASSERT_EQ(mesh_b.net().link_count(), 1u);
    EXPECT_EQ(mesh_a.net().links()[0].remote, 1u);

    ASSERT_TRUE(a1.send(1, 30, seq_payload(1)));
    step_all(4);
    ASSERT_EQ(b1.frames.size(), 1u);
    EXPECT_TRUE(a1.frames.empty());
    EXPECT_EQ(mesh_a.actor_count(), 1u);
    EXPECT_EQ(mesh_b.actor_count(), 1u);
  }

  /// ------------------------------------------------------------ security seam

  namespace {

    /// proves the AUTH gate: each side sends the key, expects the key back
    class test_psk_security final : public link_security {
     public:
      explicit test_psk_security(std::string key)
          : key(std::move(key)) {}

      std::string_view name() const override { return "test-psk"; }

      auth_result begin_auth(peer_mesh& mesh, link_record& link) override {
        const ostd::vector<uint8_t> blob(key.begin(), key.end());
        mesh.send_link_auth(link, blob);
        return auth_result::PENDING;
      }

      auth_result on_auth_frame(peer_mesh& mesh, link_record& link, std::span<const uint8_t> payload) override {
        const bool match = std::ranges::equal(payload, std::span<const char>(key.data(), key.size()),
                                              [](uint8_t a, char b) { return a == static_cast<uint8_t>(b); });
        return match ? auth_result::ESTABLISHED : auth_result::FAILED;
      }

     private:
      std::string key;
    };

    /// proves the transform plumbing: xor keystream, in-place both directions
    class test_xor_security final : public link_security {
     public:
      explicit test_xor_security(uint8_t key)
          : key(key) {}

      std::string_view name() const override { return "test-xor"; }

      auth_result begin_auth(peer_mesh& mesh, link_record& link) override { return auth_result::ESTABLISHED; }
      auth_result on_auth_frame(peer_mesh& mesh, link_record& link, std::span<const uint8_t> payload) override {
        return auth_result::FAILED;
      }

      bool encrypt(const link_record& link, ostd::vector<uint8_t>& payload) override {
        for (uint8_t& b : payload) {
          b ^= key;
        }
        return true;
      }
      bool decrypt(const link_record& link, std::span<uint8_t> payload) override {
        for (uint8_t& b : payload) {
          b ^= key;
        }
        return true;
      }

     private:
      uint8_t key;
    };

    struct dual_mesh {
      explicit dual_mesh(scope<link_security> sec_a, scope<link_security> sec_b, const peer_mesh_config& cfg = fast_cfg())
          : fabric(5), port_a(fabric), port_b(fabric), mesh_a("a", cfg), mesh_b("b", cfg) {
        mesh_a.register_transport(port_a);
        mesh_b.register_transport(port_b);
        if (sec_a != nullptr) {
          mesh_a.set_security(std::move(sec_a));
        }
        if (sec_b != nullptr) {
          mesh_b.set_security(std::move(sec_b));
        }
        a = static_cast<test_actor*>(&mesh_a.spawn_actor(make_scope<test_actor>(), 1));
        b = static_cast<test_actor*>(&mesh_b.spawn_actor(make_scope<test_actor>(), 2));
        fabric.configure_endpoint(1, false);
        b->open_listener(net_address::memory_endpoint(1));
        link_id = a->open_link(net_address::memory_endpoint(1));
      }

      void step(size_t ticks = 1) {
        for (size_t i = 0; i < ticks; ++i) {
          now += microseconds{ 1000 };
          mesh_a.tick(now);
          mesh_b.tick(now);
          fabric.tick(now);
        }
      }

      bool both_up() {
        return mesh_a.net().link_between(1, 2) != nullptr && mesh_b.net().link_between(2, 1) != nullptr;
      }

      microseconds now{ 0 };
      memory_fabric fabric;
      fabric_port port_a;
      fabric_port port_b;
      peer_mesh mesh_a;
      peer_mesh mesh_b;
      test_actor* a = nullptr;
      test_actor* b = nullptr;
      natural_t link_id = 0;
    };

  }  // namespace

  TEST_F(peer_mesh_tests, psk_auth_gates_link_up_and_matching_keys_pass) {
    dual_mesh sim(make_scope<test_psk_security>("sesame"), make_scope<test_psk_security>("sesame"));

    /// hellos land first: the link must pass through AUTHENTICATING before UP
    bool saw_authenticating = false;
    for (int i = 0; i < 32 && !sim.both_up(); ++i) {
      sim.step();
      if (const link_record* record = sim.mesh_a.net().link(sim.link_id);
          record != nullptr && record->state == link_state::AUTHENTICATING) {
        saw_authenticating = true;
      }
    }
    EXPECT_TRUE(sim.both_up());
    EXPECT_TRUE(saw_authenticating);

    ASSERT_TRUE(sim.a->send(2, 20, seq_payload(4)));
    sim.step(4);
    ASSERT_EQ(sim.b->frames.size(), 1u);
    EXPECT_EQ(sim.b->frames[0].payload, seq_payload(4));
  }

  TEST_F(peer_mesh_tests, psk_wrong_key_fails_with_security_error) {
    dual_mesh sim(make_scope<test_psk_security>("sesame"), make_scope<test_psk_security>("swordfish"));
    sim.step(32);

    EXPECT_FALSE(sim.both_up());
    EXPECT_EQ(sim.mesh_a.net().link_count(), 0u);
    EXPECT_EQ(sim.mesh_b.net().link_count(), 0u);
    EXPECT_GE(sim.mesh_a.counters().security_failures + sim.mesh_b.counters().security_failures, 1u);
    ASSERT_FALSE(sim.a->downs.empty());
  }

  TEST_F(peer_mesh_tests, xor_transforms_round_trip_and_control_bypasses_encryption) {
    dual_mesh matched(make_scope<test_xor_security>(0x5C), make_scope<test_xor_security>(0x5C));
    matched.step(8);
    ASSERT_TRUE(matched.both_up());

    const ostd::vector<uint8_t> payload = seq_payload(9, 48);
    ASSERT_TRUE(matched.a->send(2, 21, payload));
    matched.step(4);
    ASSERT_EQ(matched.b->frames.size(), 1u);
    EXPECT_EQ(matched.b->frames[0].payload, payload);

    /// mismatched keys: keepalive (control, exempt) keeps the link alive while app
    ///  payloads garble — proof the wire bytes were actually transformed
    dual_mesh garbled(make_scope<test_xor_security>(0x5C), make_scope<test_xor_security>(0xA3));
    garbled.step(8);
    ASSERT_TRUE(garbled.both_up());

    ASSERT_TRUE(garbled.a->send(2, 21, payload));
    garbled.step(4);
    ASSERT_EQ(garbled.b->frames.size(), 1u);
    EXPECT_EQ(garbled.b->frames[0].payload.size(), payload.size());
    EXPECT_NE(garbled.b->frames[0].payload, payload);

    /// outlive several keepalive windows on the mismatched pair
    garbled.step(512);
    EXPECT_TRUE(garbled.both_up());
    EXPECT_TRUE(garbled.a->downs.empty());
  }

  TEST_F(peer_mesh_tests, none_security_skips_auth_state_entirely) {
    mesh_sim_fixture sim(2, 1, fast_cfg());
    const natural_t link_id = sim.link(1, 2);

    bool saw_authenticating = false;
    ASSERT_TRUE(sim.step_until([&] {
      if (const link_record* record = sim.mesh().net().link(link_id);
          record != nullptr && record->state == link_state::AUTHENTICATING) {
        saw_authenticating = true;
      }
      return link_up_both(sim, 1, 2);
    }));
    EXPECT_FALSE(saw_authenticating);
  }

  TEST_F(peer_mesh_tests, destroy_actor_closes_its_links) {
    mesh_sim_fixture sim(3);
    sim.link(1, 2);
    sim.link(1, 3);
    ASSERT_TRUE(sim.step_until([&] { return link_up_both(sim, 1, 2) && link_up_both(sim, 1, 3); }));
    ASSERT_EQ(sim.mesh().net().link_count(), 4u);

    sim.mesh().destroy_actor(1);
    sim.step(microseconds{ 1000 }, 8);

    EXPECT_EQ(sim.mesh().actor_count(), 2u);
    EXPECT_EQ(sim.mesh().net().link_count(), 0u);
    EXPECT_FALSE(sim.actor(2).downs.empty());
    EXPECT_FALSE(sim.actor(3).downs.empty());
    EXPECT_FALSE(sim.mesh().graph().contains(1));
  }

}  // namespace other
