/**
 * \file network/transport_conformance_tests.cpp
 *
 * the transport conformance battery: one set of expectations, run over the in-tree
 * transports. reliable profile: memory (mesh_sim_fixture legs live in
 * peer_mesh_tests.cpp) + real localhost tcp via the dual-instance harness. datagram
 * profile: memory-datagram + real localhost udp. byte rows exercise providers alone;
 * framing rows exercise the mesh riding the transport. row 13
 * (session_join_refused_on_datagram_link) waits for the session actor in M2 — link
 * caps propagation, its precondition, is proven here.
 */
#include <algorithm>
#include <numeric>

#include <gtest/gtest.h>

#include "network/frame.hpp"
#include "network/memory/memory_fabric.hpp"
#include "peer_mesh/mesh_messages.hpp"
#include "peer_mesh/mesh_router.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "network/mesh_sim_fixture.hpp"
#include "network/socket_mesh_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class transport_conformance_tests : public other_test {};

  namespace {

    std::vector<uint8_t> pattern_bytes(size_t size, uint32_t salt = 0) {
      std::vector<uint8_t> bytes(size);
      for (size_t i = 0; i < size; ++i) {
        bytes[i] = static_cast<uint8_t>((i * 31 + (i >> 8) + salt) & 0xFF);
      }
      return bytes;
    }

    ostd::vector<uint8_t> hello_frame(node_id node) {
      const mesh_link_hello hello{ .node = node };
      return write_frame(static_cast<uint16_t>(mesh_message::LINK_HELLO), serialize_direct(hello));
    }

    test_actor& spawn_recorder(peer_mesh& mesh, node_id id) {
      return static_cast<test_actor&>(mesh.spawn_actor(make_scope<test_actor>(), id));
    }

    /// establish one raw tcp pair (no mesh): returns {dial conn on b, accepted conn on a}
    struct raw_pair {
      natural_t dial_conn = 0;
      natural_t accepted_conn = 0;
      natural_t listener_id = 0;
    };

    bool establish_raw_pair(socket_net_instance& a, socket_net_instance& b, uint16_t port, raw_pair& out) {
      auto listen = a.raw_listen(port);
      if (!pump_until({ &a, &b }, [&] { return a.ack_result(listen.ack_id).has_value(); })) {
        return false;
      }
      if (a.ack_result(listen.ack_id) != opt<uint8_t>{ 1 }) {
        return false;
      }

      const natural_t dial_conn = b.raw_connect(port);
      const size_t prior_accepts = a.opened_notes.size();
      if (!pump_until({ &a, &b }, [&] {
            return b.has_opened_note(dial_conn) && a.accepted_note_for_listener(listen.listener_id) != nullptr;
          })) {
        return false;
      }

      out.dial_conn = dial_conn;
      out.listener_id = listen.listener_id;
      out.accepted_conn = a.accepted_note_for_listener(listen.listener_id)->connection_id;
      (void)prior_accepts;
      return true;
    }

  }  // namespace

  /// ------------------------------------------------------------------ reliable / tcp

  TEST_F(transport_conformance_tests, tcp_listen_accept_connect_and_notify) {
    socket_net_instance a("conf-a");
    socket_net_instance b("conf-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    raw_pair pair;
    ASSERT_TRUE(establish_raw_pair(a, b, next_test_port(), pair));

    const notification_connection_opened* dial_note = b.opened_note(pair.dial_conn);
    ASSERT_NE(dial_note, nullptr);
    EXPECT_EQ(dial_note->outbound, 1);
    EXPECT_EQ(dial_note->listener_id, 0u);

    const notification_connection_opened* accept_note = a.accepted_note_for_listener(pair.listener_id);
    ASSERT_NE(accept_note, nullptr);
    EXPECT_EQ(accept_note->outbound, 0);
    EXPECT_EQ(accept_note->remote.ip, socket_net_instance::network_system_localhost());
  }

  TEST_F(transport_conformance_tests, tcp_connect_refused_reports_failure_not_abort) {
    socket_net_instance b("conf-refused");
    ASSERT_TRUE(b.start());

    const natural_t conn = b.raw_connect(next_test_port());  // nobody listening
    ASSERT_TRUE(pump_until({ &b }, [&] { return b.closed_note(conn) != nullptr; }));
    EXPECT_EQ(b.closed_note(conn)->reason, static_cast<uint16_t>(connection_close_reason::CONNECT_FAILED));
    EXPECT_FALSE(b.has_opened_note(conn));

    /// the thread survived: a real listen still works
    auto listen = b.raw_listen(next_test_port());
    ASSERT_TRUE(pump_until({ &b }, [&] { return b.ack_result(listen.ack_id).has_value(); }));
    EXPECT_EQ(b.ack_result(listen.ack_id), opt<uint8_t>{ 1 });
  }

  TEST_F(transport_conformance_tests, tcp_exchange_1mb_byte_exact) {
    socket_net_instance a("conf-1mb-a");
    socket_net_instance b("conf-1mb-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    recording_sink a_rx;
    recording_sink b_rx;
    a.tcp->register_packet_sink(&a_rx);
    b.tcp->register_packet_sink(&b_rx);

    raw_pair pair;
    ASSERT_TRUE(establish_raw_pair(a, b, next_test_port(), pair));

    constexpr size_t kTotal = 1024 * 1024;
    constexpr size_t kChunk = 64 * 1024;
    const std::vector<uint8_t> to_a = pattern_bytes(kTotal, 1);
    const std::vector<uint8_t> to_b = pattern_bytes(kTotal, 2);

    for (size_t off = 0; off < kTotal; off += kChunk) {
      b.raw_tx(pair.dial_conn, std::span<const uint8_t>(to_a.data() + off, kChunk));
      a.raw_tx(pair.accepted_conn, std::span<const uint8_t>(to_b.data() + off, kChunk));
    }

    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a_rx.stream_of(pair.accepted_conn).size() >= kTotal && b_rx.stream_of(pair.dial_conn).size() >= kTotal;
    }, std::chrono::milliseconds(10000)));

    EXPECT_EQ(a_rx.stream_of(pair.accepted_conn), to_a);
    EXPECT_EQ(b_rx.stream_of(pair.dial_conn), to_b);
  }

  TEST_F(transport_conformance_tests, tcp_frames_survive_dribble_and_coalesce) {
    socket_net_instance a("conf-frame-a");
    socket_net_instance b("conf-frame-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor = spawn_recorder(a.mesh, 1);
    ASSERT_NE(actor.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })), 0u);

    /// raw endpoint speaking the mesh link protocol by hand
    const natural_t raw_conn = b.raw_connect(port);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(raw_conn) && a.mesh.net().link_count() == 1; }));

    /// hello fed one byte at a time: the reader must reassemble across 60-odd chunks
    const ostd::vector<uint8_t> hello = hello_frame(99);
    for (const uint8_t byte : hello) {
      b.raw_tx(raw_conn, std::span<const uint8_t>(&byte, 1));
    }
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.mesh.net().link_between(1, 99) != nullptr; }));

    /// three frames coalesced into one tx; the reader must split them
    const std::vector<uint8_t> p1 = pattern_bytes(64, 10);
    const std::vector<uint8_t> p2 = pattern_bytes(256, 20);
    const std::vector<uint8_t> p3 = pattern_bytes(9, 30);
    ostd::vector<uint8_t> coalesced;
    for (const auto* payload : { &p1, &p2, &p3 }) {
      const ostd::vector<uint8_t> frame = write_frame(0x0042, std::span<const uint8_t>(payload->data(), payload->size()));
      coalesced.insert(coalesced.end(), frame.begin(), frame.end());
    }
    b.raw_tx(raw_conn, coalesced);

    /// one 64 KiB frame in awkward 7000-byte slices: reassembly across many chunks
    const std::vector<uint8_t> big = pattern_bytes(64 * 1024, 40);
    const ostd::vector<uint8_t> big_frame = write_frame(0x0043, std::span<const uint8_t>(big.data(), big.size()));
    for (size_t off = 0; off < big_frame.size(); off += 7000) {
      const size_t len = std::min<size_t>(7000, big_frame.size() - off);
      b.raw_tx(raw_conn, std::span<const uint8_t>(big_frame.data() + off, len));
    }

    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return actor.frames.size() >= 4; }));
    ASSERT_EQ(actor.frames.size(), 4u);
    EXPECT_EQ(actor.frames[0].src, 99u);
    EXPECT_EQ(actor.frames[0].net_id, 0x0042u);
    EXPECT_TRUE(std::ranges::equal(actor.frames[0].payload, p1));
    EXPECT_TRUE(std::ranges::equal(actor.frames[1].payload, p2));
    EXPECT_TRUE(std::ranges::equal(actor.frames[2].payload, p3));
    EXPECT_EQ(actor.frames[3].net_id, 0x0043u);
    EXPECT_TRUE(std::ranges::equal(actor.frames[3].payload, big));
  }

  TEST_F(transport_conformance_tests, tcp_close_tears_down_route_and_notifies_both_sides) {
    socket_net_instance a("conf-close-a");
    socket_net_instance b("conf-close-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    raw_pair pair;
    ASSERT_TRUE(establish_raw_pair(a, b, next_test_port(), pair));
    EXPECT_EQ(a.thread.active_route_count(), 2u);  // listener + accepted conn
    EXPECT_EQ(b.thread.active_route_count(), 1u);  // dialed conn

    b.raw_close(pair.dial_conn);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return b.closed_note(pair.dial_conn) != nullptr && a.closed_note(pair.accepted_conn) != nullptr;
    }));

    EXPECT_EQ(b.closed_note(pair.dial_conn)->reason, static_cast<uint16_t>(connection_close_reason::LOCAL_CLOSE));
    EXPECT_EQ(a.closed_note(pair.accepted_conn)->reason, static_cast<uint16_t>(connection_close_reason::REMOTE_CLOSED));

    /// routes died with the connection, not at shutdown
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.thread.active_route_count() == 1 && b.thread.active_route_count() == 0;
    }));
  }

  TEST_F(transport_conformance_tests, tcp_routes_do_not_accumulate_across_cycles) {
    socket_net_instance a("conf-cycle-a");
    socket_net_instance b("conf-cycle-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    auto listen = a.raw_listen(port);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.ack_result(listen.ack_id) == opt<uint8_t>{ 1 }; }));

    for (int cycle = 0; cycle < 20; ++cycle) {
      const size_t accepts_before = a.opened_notes.size();
      const natural_t conn = b.raw_connect(port);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] {
        return b.has_opened_note(conn) && a.opened_notes.size() > accepts_before;
      })) << "cycle " << cycle;

      b.raw_close(conn);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] {
        return b.closed_note(conn) != nullptr && a.thread.active_route_count() == 1 && b.thread.active_route_count() == 0;
      })) << "cycle " << cycle;
    }

    EXPECT_EQ(a.thread.active_route_count(), 1u);
    EXPECT_EQ(b.thread.active_route_count(), 0u);
    EXPECT_EQ(a.closed_notes.size(), 20u);
  }

  TEST_F(transport_conformance_tests, tcp_oversize_and_malformed_frames_close_not_assert) {
    socket_net_instance a("conf-poison-a");
    socket_net_instance b("conf-poison-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    spawn_recorder(a.mesh, 1).open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }));

    /// oversize: a length field far past max-frame poisons the stream
    {
      const natural_t conn = b.raw_connect(port);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(conn) && a.mesh.net().link_count() == 1; }));

      const uint8_t oversize[8] = { 0xFF, 0xFF, 0xFF, 0x7F, 0x42, 0x00, 0x00, 0x00 };
      b.raw_tx(conn, oversize);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.mesh.net().link_count() == 0; }));
      EXPECT_GE(a.mesh.counters().protocol_errors, 1u);
    }

    /// malformed: a length below the frame floor is equally fatal to the link only
    {
      const natural_t conn = b.raw_connect(port);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(conn) && a.mesh.net().link_count() == 1; }));

      const uint8_t malformed[8] = { 0x02, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00 };
      b.raw_tx(conn, malformed);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.mesh.net().link_count() == 0; }));
      EXPECT_GE(a.mesh.counters().protocol_errors, 2u);
    }

    /// the process and the listener both survived: a fresh handshake still completes
    {
      const natural_t conn = b.raw_connect(port);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(conn) && a.mesh.net().link_count() == 1; }));
      const ostd::vector<uint8_t> hello = hello_frame(77);
      b.raw_tx(conn, hello);
      ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.mesh.net().link_between(1, 77) != nullptr; }));
    }
  }

  TEST_F(transport_conformance_tests, tcp_send_queue_backpressure_closes_instead_of_growing) {
    socket_net_instance b("conf-backpressure");
    ASSERT_TRUE(b.start());

    /// a peer that accepts and then never reads: kernel buffers fill, our queue grows
    const uint16_t port = next_test_port();
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor(io, asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), port));
    asio::ip::tcp::socket peer(io);
    std::atomic<bool> accepted = false;
    acceptor.async_accept(peer, [&](const asio::error_code& ec) { accepted = !ec; });

    const natural_t conn = b.raw_connect(port);
    ASSERT_TRUE(pump_until({ &b }, [&] {
      io.poll();
      return accepted.load() && b.has_opened_note(conn);
    }));

    const std::vector<uint8_t> chunk = pattern_bytes(64 * 1024);
    for (int i = 0; i < 128; ++i) {  // 8 MiB total against a 4 MiB queue cap
      b.raw_tx(conn, chunk);
    }

    ASSERT_TRUE(pump_until({ &b }, [&] {
      io.poll();
      return b.closed_note(conn) != nullptr;
    }, std::chrono::milliseconds(10000)));
    EXPECT_EQ(b.closed_note(conn)->reason, static_cast<uint16_t>(connection_close_reason::BACKPRESSURE));
  }

  TEST_F(transport_conformance_tests, link_handshake_and_keepalive_over_tcp) {
    socket_net_instance a("conf-mesh-a");
    socket_net_instance b("conf-mesh-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor_a = spawn_recorder(a.mesh, 1);
    test_actor& actor_b = spawn_recorder(b.mesh, 2);

    ASSERT_NE(actor_a.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })), 0u);
    ASSERT_NE(actor_b.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })), 0u);

    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.mesh.net().link_between(1, 2) != nullptr && b.mesh.net().link_between(2, 1) != nullptr;
    }));
    EXPECT_EQ(actor_a.ups.size(), 1u);
    EXPECT_EQ(actor_b.ups.size(), 1u);
    EXPECT_TRUE(a.mesh.net().link_between(1, 2)->caps.reliable);
    EXPECT_TRUE(a.mesh.net().link_between(1, 2)->caps.ordered);

    /// application frames ride the up link byte-exact, both directions
    const std::vector<uint8_t> ping_payload = pattern_bytes(300, 5);
    EXPECT_TRUE(actor_b.send(1, 0x0100, ping_payload));
    EXPECT_TRUE(actor_a.send(2, 0x0101, ping_payload));
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return !actor_a.frames.empty() && !actor_b.frames.empty(); }));
    EXPECT_TRUE(std::ranges::equal(actor_a.frames[0].payload, ping_payload));
    EXPECT_EQ(actor_a.frames[0].src, 2u);
    EXPECT_TRUE(std::ranges::equal(actor_b.frames[0].payload, ping_payload));

    /// idle past keepalive: pings keep the link up and feed rtt — no teardown
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      const link_record* ab = a.mesh.net().link_between(1, 2);
      return ab != nullptr && ab->rtt.count() > 0;
    }, std::chrono::milliseconds(4000)));
    EXPECT_NE(a.mesh.net().link_between(1, 2), nullptr);
    EXPECT_NE(b.mesh.net().link_between(2, 1), nullptr);
    EXPECT_TRUE(actor_a.downs.empty());
    EXPECT_TRUE(actor_b.downs.empty());
  }

  TEST_F(transport_conformance_tests, tcp_listen_on_taken_port_is_error_not_death) {
    socket_net_instance a("conf-port");
    ASSERT_TRUE(a.start());

    const uint16_t port = next_test_port();
    auto first = a.raw_listen(port);
    ASSERT_TRUE(pump_until({ &a }, [&] { return a.ack_result(first.ack_id).has_value(); }));
    EXPECT_EQ(a.ack_result(first.ack_id), opt<uint8_t>{ 1 });

    auto second = a.raw_listen(port);
    ASSERT_TRUE(pump_until({ &a }, [&] { return a.ack_result(second.ack_id).has_value(); }));
    EXPECT_EQ(a.ack_result(second.ack_id), opt<uint8_t>{ 0 });

    /// alive and functional afterwards
    auto third = a.raw_listen(next_test_port());
    ASSERT_TRUE(pump_until({ &a }, [&] { return a.ack_result(third.ack_id).has_value(); }));
    EXPECT_EQ(a.ack_result(third.ack_id), opt<uint8_t>{ 1 });
  }

  TEST_F(transport_conformance_tests, bus_flood_drains_within_budget) {
    socket_net_instance a("conf-flood");
    ASSERT_TRUE(a.start());

    /// 2000 junk commands ahead of a real one: the per-pump drain must chew through
    const std::vector<uint8_t> junk = pattern_bytes(64);
    for (int i = 0; i < 2000; ++i) {
      a.raw_tx(999'999, junk);  // unknown conn: warn-and-continue path
    }

    auto listen = a.raw_listen(next_test_port());
    ASSERT_TRUE(pump_until({ &a }, [&] { return a.ack_result(listen.ack_id).has_value(); }, std::chrono::milliseconds(5000)));
    EXPECT_EQ(a.ack_result(listen.ack_id), opt<uint8_t>{ 1 });
  }

  /// ---------------------------------------------------------------- datagram profile

  TEST_F(transport_conformance_tests, datagram_link_establishes_via_hello_and_reports_caps_udp) {
    socket_net_instance a("conf-udp-a");
    socket_net_instance b("conf-udp-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor_a = spawn_recorder(a.mesh, 1);
    test_actor& actor_b = spawn_recorder(b.mesh, 2);

    ASSERT_NE(actor_a.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_NE(actor_b.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);

    /// the unknown-endpoint synth happens on a's side when b's hello lands; the hello
    ///  IS the establishment — there is no transport handshake
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.mesh.net().link_between(1, 2) != nullptr && b.mesh.net().link_between(2, 1) != nullptr;
    }));

    const link_record* ab = a.mesh.net().link_between(1, 2);
    const link_record* ba = b.mesh.net().link_between(2, 1);
    EXPECT_FALSE(ab->caps.reliable);
    EXPECT_FALSE(ab->caps.ordered);
    EXPECT_EQ(ab->caps.max_frame_size, udp_transport_provider::kDefaultMaxDatagramBytes);
    EXPECT_FALSE(ba->caps.reliable);
    EXPECT_FALSE(ba->caps.ordered);
    EXPECT_EQ(ba->caps.max_frame_size, udp_transport_provider::kDefaultMaxDatagramBytes);
  }

  TEST_F(transport_conformance_tests, datagram_link_establishes_via_hello_and_reports_caps_memory) {
    mesh_sim_fixture sim(2);
    sim.link(1, 2, {}, {}, /*datagram=*/true);
    ASSERT_TRUE(sim.step_until([&] {
      return sim.mesh().net().link_between(1, 2) != nullptr && sim.mesh().net().link_between(2, 1) != nullptr;
    }));

    const link_record* ab = sim.mesh().net().link_between(1, 2);
    EXPECT_FALSE(ab->caps.reliable);
    EXPECT_FALSE(ab->caps.ordered);
  }

  TEST_F(transport_conformance_tests, udp_frames_round_trip_one_datagram_each) {
    socket_net_instance a("conf-udp-rt-a");
    socket_net_instance b("conf-udp-rt-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor_a = spawn_recorder(a.mesh, 1);
    test_actor& actor_b = spawn_recorder(b.mesh, 2);
    ASSERT_NE(actor_a.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_NE(actor_b.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.mesh.net().link_between(1, 2) != nullptr && b.mesh.net().link_between(2, 1) != nullptr;
    }));

    constexpr size_t kFrames = 20;
    for (size_t i = 0; i < kFrames; ++i) {
      const std::vector<uint8_t> payload = pattern_bytes(200 + i, static_cast<uint32_t>(i));
      EXPECT_TRUE(actor_b.send(1, static_cast<uint16_t>(0x0200 + i), payload));
      EXPECT_TRUE(actor_a.send(2, static_cast<uint16_t>(0x0300 + i), payload));
    }

    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return actor_a.frames.size() >= kFrames && actor_b.frames.size() >= kFrames;
    }, std::chrono::milliseconds(5000)));

    for (size_t i = 0; i < kFrames; ++i) {
      EXPECT_TRUE(std::ranges::equal(actor_a.frames[i].payload, pattern_bytes(200 + i, static_cast<uint32_t>(i))));
      EXPECT_EQ(actor_a.frames[i].net_id, 0x0200 + i);
      EXPECT_EQ(actor_a.frames[i].src, 2u);
    }
  }

  TEST_F(transport_conformance_tests, udp_oversize_tx_refused_not_fragmented) {
    socket_net_instance a("conf-udp-big-a");
    socket_net_instance b("conf-udp-big-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor_a = spawn_recorder(a.mesh, 1);
    test_actor& actor_b = spawn_recorder(b.mesh, 2);
    ASSERT_NE(actor_a.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_NE(actor_b.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.mesh.net().link_between(1, 2) != nullptr && b.mesh.net().link_between(2, 1) != nullptr;
    }));

    /// the mesh refuses at the caps ceiling before the provider ever sees it
    const std::vector<uint8_t> too_big = pattern_bytes(1500);
    const natural_t refused_before = b.mesh.counters().refused_sends;
    EXPECT_FALSE(actor_b.send(1, 0x0400, too_big));
    EXPECT_GT(b.mesh.counters().refused_sends, refused_before);

    /// a raw consumer pushing an oversize datagram hits the provider's own refusal
    const natural_t raw_conn = b.raw_connect(port, "udp");
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(raw_conn); }));
    const std::vector<uint8_t> oversize_datagram = pattern_bytes(2000);
    b.raw_tx(raw_conn, oversize_datagram);
    ASSERT_TRUE(pump_until({ &b }, [&] { return b.udp->oversize_tx_refusals() >= 1; }));

    /// nothing fragmented, nothing delivered
    a.pump();
    EXPECT_TRUE(std::ranges::none_of(actor_a.frames, [](const recorded_frame& f) { return f.net_id == 0x0400; }));
  }

  TEST_F(transport_conformance_tests, udp_keepalive_is_the_only_liveness_signal) {
    socket_net_instance a("conf-udp-live-a");
    socket_net_instance b("conf-udp-live-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    const uint16_t port = next_test_port();
    test_actor& actor_a = spawn_recorder(a.mesh, 1);
    test_actor& actor_b = spawn_recorder(b.mesh, 2);
    ASSERT_NE(actor_a.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_NE(actor_b.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }), "udp"), 0u);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return a.mesh.net().link_between(1, 2) != nullptr && b.mesh.net().link_between(2, 1) != nullptr;
    }));

    /// b goes silent (its mesh stops ticking entirely); no transport event exists to
    ///  notice — a's keepalive timeout is the only reaper
    ASSERT_TRUE(pump_until({ &a }, [&] { return !actor_a.downs.empty(); }, std::chrono::milliseconds(3000)));
    EXPECT_EQ(actor_a.downs[0].reason, link_close_reason::KEEPALIVE_TIMEOUT);
    (void)actor_b;
  }

  TEST_F(transport_conformance_tests, datagram_keepalive_reaps_muted_memory_link) {
    peer_mesh_config cfg;
    cfg.handshake_timeout = microseconds{ 100'000 };
    cfg.keepalive_idle = microseconds{ 50'000 };
    cfg.link_timeout = microseconds{ 200'000 };
    mesh_sim_fixture sim(2, 1, cfg);
    const natural_t link_id = sim.link(1, 2, {}, {}, /*datagram=*/true);
    ASSERT_TRUE(sim.step_until([&] { return sim.mesh().net().link_between(1, 2) != nullptr && sim.mesh().net().link_between(2, 1) != nullptr; }));

    const link_record* record = sim.mesh().net().link(link_id);
    ASSERT_NE(record, nullptr);
    sim.fabric.set_mute(record->connection_id, true, true);

    ASSERT_TRUE(sim.step_until([&] { return !sim.actor(1).downs.empty() && !sim.actor(2).downs.empty(); }, 600));
    EXPECT_EQ(sim.actor(1).downs[0].reason, link_close_reason::KEEPALIVE_TIMEOUT);
  }

  /// ------------------------------------------------------- cross-profile: the tap case

  TEST_F(transport_conformance_tests, one_network_spans_memory_and_tcp_links) {
    /// declared before the instances: transports must outlive the mesh that links
    ///  over them (the mesh closes its links in its destructor)
    memory_fabric fabric(7);
    fabric_port fabric_a(fabric);

    socket_net_instance a("conf-span-a");
    socket_net_instance b("conf-span-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    /// instance a: three actors on memory links (the simulated MANET shape), one of
    ///  which egresses over real tcp to instance b — ONE network holding both
    a.mesh.register_transport(fabric_a);

    test_actor& actor_1 = spawn_recorder(a.mesh, 1);
    test_actor& actor_2 = spawn_recorder(a.mesh, 2);
    test_actor& actor_3 = spawn_recorder(a.mesh, 3);
    test_actor& actor_9 = spawn_recorder(b.mesh, 9);

    auto pump_all = [&] {
      a.pump();
      b.pump();
      fabric.tick(a.now());
    };
    auto pump_all_until = [&](auto&& done, std::chrono::milliseconds deadline = std::chrono::milliseconds(4000)) {
      const auto start = std::chrono::steady_clock::now();
      while (std::chrono::steady_clock::now() - start < deadline) {
        pump_all();
        if (done()) {
          return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      return done();
    };

    fabric.configure_endpoint(1, false);
    fabric.configure_endpoint(2, false);
    ASSERT_NE(actor_2.open_listener(net_address::memory_endpoint(1)), 0u);
    const natural_t link_1_2 = actor_1.open_link(net_address::memory_endpoint(1));
    ASSERT_NE(link_1_2, 0u);
    ASSERT_NE(actor_3.open_listener(net_address::memory_endpoint(2)), 0u);
    const natural_t link_2_3 = actor_2.open_link(net_address::memory_endpoint(2));
    ASSERT_NE(link_2_3, 0u);

    const uint16_t port = next_test_port();
    ASSERT_NE(actor_9.open_listener(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port })), 0u);
    const natural_t link_3_9 = actor_3.open_link(net_address::ip_endpoint({ socket_net_instance::network_system_localhost(), port }));
    ASSERT_NE(link_3_9, 0u);

    ASSERT_TRUE(pump_all_until([&] {
      return a.mesh.net().link_between(1, 2) != nullptr &&
        a.mesh.net().link_between(2, 3) != nullptr &&
        a.mesh.net().link_between(3, 9) != nullptr &&
        b.mesh.net().link_between(9, 3) != nullptr;
    }));

    /// static routes: every seat knows its next hop toward 9
    scope<static_route_router> routes = make_scope<static_route_router>();
    routes->set_route(1, 9, link_1_2);
    routes->set_route(2, 9, link_2_3);
    routes->set_route(3, 9, link_3_9);
    a.mesh.set_router(std::move(routes));

    const std::vector<uint8_t> payload = pattern_bytes(512, 99);
    ASSERT_TRUE(actor_1.send(9, 0x0500, payload));

    ASSERT_TRUE(pump_all_until([&] { return !actor_9.frames.empty(); }));
    EXPECT_EQ(actor_9.frames[0].src, 1u);
    EXPECT_EQ(actor_9.frames[0].net_id, 0x0500u);
    EXPECT_TRUE(std::ranges::equal(actor_9.frames[0].payload, payload));

    /// relays observed nothing at their mailboxes: forwarding is not delivery
    EXPECT_TRUE(actor_2.frames.empty());
    EXPECT_TRUE(actor_3.frames.empty());
  }

  TEST_F(transport_conformance_tests, per_link_sink_scoping_holds_on_sockets) {
    socket_net_instance a("conf-scope-a");
    socket_net_instance b("conf-scope-b");
    ASSERT_TRUE(a.start());
    ASSERT_TRUE(b.start());

    recording_sink wide;
    a.tcp->register_packet_sink(&wide);

    const uint16_t port = next_test_port();
    auto listen = a.raw_listen(port);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return a.ack_result(listen.ack_id) == opt<uint8_t>{ 1 }; }));

    const natural_t conn_1 = b.raw_connect(port);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(conn_1) && a.opened_notes.size() >= 1; }));
    const natural_t a_conn_1 = a.opened_notes[0].connection_id;

    const natural_t conn_2 = b.raw_connect(port);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return b.has_opened_note(conn_2) && a.opened_notes.size() >= 2; }));
    const natural_t a_conn_2 = a.opened_notes[1].connection_id;

    /// first bytes land before the conn-scoped sink exists: the rx hold must replay
    ///  them to it, so the scoped view starts at the connection's first byte
    const std::vector<uint8_t> first = pattern_bytes(96, 1);
    b.raw_tx(conn_1, first);
    ASSERT_TRUE(pump_until({ &a, &b }, [&] { return wide.stream_of(a_conn_1).size() >= first.size(); }));

    recording_sink scoped;
    conn_sink_binding binding{ a_conn_1, &scoped };
    a.tcp->register_conn_sink(&binding);

    const std::vector<uint8_t> second = pattern_bytes(128, 2);
    const std::vector<uint8_t> other_conn_bytes = pattern_bytes(64, 3);
    b.raw_tx(conn_1, second);
    b.raw_tx(conn_2, other_conn_bytes);

    ASSERT_TRUE(pump_until({ &a, &b }, [&] {
      return scoped.stream_of(a_conn_1).size() >= first.size() + second.size() &&
        wide.stream_of(a_conn_2).size() >= other_conn_bytes.size();
    }));

    /// scoped: exactly its connection, from byte zero, in order
    std::vector<uint8_t> expected = first;
    expected.insert(expected.end(), second.begin(), second.end());
    EXPECT_EQ(scoped.stream_of(a_conn_1), expected);
    EXPECT_TRUE(scoped.stream_of(a_conn_2).empty());

    /// wide: everything on the transport, as the wire carried it
    EXPECT_EQ(wide.stream_of(a_conn_1), expected);
    EXPECT_EQ(wide.stream_of(a_conn_2), other_conn_bytes);

    /// reclaim discipline: tombstone, then hold the binding until the pump moved on
    a.tcp->unregister_conn_sink(&binding);
    const uint64_t epoch = a.thread.reclamation_epoch();
    ASSERT_TRUE(pump_until({ &a }, [&] { return a.thread.reclamation_epoch() > epoch + 1; }));
  }

}  // namespace other