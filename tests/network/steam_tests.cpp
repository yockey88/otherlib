/**
 * \file network/steam_tests.cpp
 **/
#include <gtest/gtest.h>

#include "network/memory/memory_fabric.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "steam/steam_context.hpp"
#include "steam/steam_link_transport.hpp"
#include "steam/steam_lobby.hpp"

#include "network/mesh_sim_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class steam_tests : public other_test {};

  /// ------------------------------------------------------------ fragmentation

  TEST_F(steam_tests, fragment_reassembly_round_trips_2mib_frame) {
    ostd::vector<uint8_t> blob(2 * 1024 * 1024);
    for (size_t i = 0; i < blob.size(); ++i) {
      blob[i] = static_cast<uint8_t>((i * 31) ^ (i >> 8));
    }

    const ostd::vector<ostd::vector<uint8_t>> chunks = fragment_blob(blob);
    EXPECT_EQ(chunks.size(), 5u);  // 2 MiB over 448 KiB chunks
    for (const auto& chunk : chunks) {
      EXPECT_LE(chunk.size(), kSteamFragmentHeader + kSteamFragmentPayload);
    }

    fragment_accumulator assembler;
    opt<ostd::vector<uint8_t>> result;
    for (size_t i = 0; i < chunks.size(); ++i) {
      result = assembler.feed(chunks[i]);
      EXPECT_EQ(result.has_value(), i == chunks.size() - 1);
    }
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, blob);

    /// small and empty blobs are single final chunks
    const ostd::vector<uint8_t> tiny{ 1, 2, 3 };
    const auto tiny_chunks = fragment_blob(tiny);
    ASSERT_EQ(tiny_chunks.size(), 1u);
    EXPECT_EQ(assembler.feed(tiny_chunks[0]), opt<ostd::vector<uint8_t>>{ tiny });
    const auto empty_chunks = fragment_blob({});
    ASSERT_EQ(empty_chunks.size(), 1u);
    EXPECT_EQ(assembler.feed(empty_chunks[0]), opt<ostd::vector<uint8_t>>{ ostd::vector<uint8_t>{} });

    /// a truncated header resets instead of corrupting the next blob
    EXPECT_EQ(assembler.feed(ostd::vector<uint8_t>{ 0x01 }), std::nullopt);
    EXPECT_EQ(assembler.feed(tiny_chunks[0]), opt<ostd::vector<uint8_t>>{ tiny });
  }

  /// ------------------------------------------------------------ attestation

  namespace {

    /// the §6 rule without Valve involved: a transport that swears to a remote id
    class attesting_fabric_port final : public fabric_port {
     public:
      using fabric_port::fabric_port;

      node_id attested_remote(natural_t conn_id) const override {
        auto itr = attested.find(conn_id);
        return itr != attested.end() ? itr->second : 0;
      }

      ostd::map<natural_t, node_id> attested;
    };

    struct attested_sim {
      attested_sim()
          : fabric(1), port(fabric), mesh("attested", [] {
              peer_mesh_config cfg;
              cfg.handshake_timeout = microseconds{ 100'000 };
              return cfg;
            }()) {
        mesh.register_transport(port);
        mesh.spawn_actor(make_scope<test_actor>(), 1);
        mesh.spawn_actor(make_scope<test_actor>(), 2);
        fabric.configure_endpoint(1, false);
      }

      test_actor& actor(node_id node) { return static_cast<test_actor&>(*mesh.actor(node)); }

      void step(size_t ticks) {
        for (size_t i = 0; i < ticks; ++i) {
          now += microseconds{ 1000 };
          mesh.tick(now);
          fabric.tick(now);
        }
      }

      microseconds now{ 0 };
      memory_fabric fabric;
      attesting_fabric_port port;
      peer_mesh mesh;
    };

  }  // namespace

  TEST_F(steam_tests, link_hello_mismatching_attested_identity_closes) {
    attested_sim sim;
    ASSERT_NE(sim.actor(2).open_listener(net_address::memory_endpoint(1)), 0u);
    const natural_t link_id = sim.actor(1).open_link(net_address::memory_endpoint(1));
    ASSERT_NE(link_id, 0u);

    /// the transport attests node 999; the hello will claim node 2
    sim.port.attested[sim.mesh.net().link(link_id)->connection_id] = 999;

    sim.step(16);
    ASSERT_FALSE(sim.actor(1).downs.empty());
    EXPECT_EQ(sim.actor(1).downs.front().reason, link_close_reason::SECURITY_ERROR);
    EXPECT_TRUE(sim.actor(1).ups.empty());
    EXPECT_GE(sim.mesh.counters().security_failures, 1u);
  }

  TEST_F(steam_tests, link_hello_matching_attested_identity_marks_link) {
    attested_sim sim;
    ASSERT_NE(sim.actor(2).open_listener(net_address::memory_endpoint(1)), 0u);
    const natural_t link_id = sim.actor(1).open_link(net_address::memory_endpoint(1));
    ASSERT_NE(link_id, 0u);

    sim.port.attested[sim.mesh.net().link(link_id)->connection_id] = 2;

    sim.step(16);
    ASSERT_FALSE(sim.actor(1).ups.empty());
    EXPECT_TRUE(sim.actor(1).ups.front().attested);
    /// the acceptor's conn carries no attestation — it stays self-reported
    ASSERT_FALSE(sim.actor(2).ups.empty());
    EXPECT_FALSE(sim.actor(2).ups.front().attested);
  }

  /// ------------------------------------------------------------ lobby flow

  TEST_F(steam_tests, lobby_host_flow_reaches_listening) {
    steam_lobby lobby;
    EXPECT_EQ(lobby.state(), lobby_state::IDLE);

    ASSERT_TRUE(lobby.create(1, 8));
    EXPECT_EQ(lobby.state(), lobby_state::CREATING);
    EXPECT_FALSE(lobby.create(1, 8));  // busy

    lobby.on_lobby_created(true, 42);
    EXPECT_EQ(lobby.state(), lobby_state::HOSTING);
    EXPECT_EQ(lobby.lobby_id(), 42u);

    lobby.leave();
    EXPECT_EQ(lobby.state(), lobby_state::IDLE);
    EXPECT_EQ(lobby.lobby_id(), 0u);

    /// creation failure lands back in IDLE, re-creatable
    ASSERT_TRUE(lobby.create(1, 8));
    lobby.on_lobby_created(false, 0);
    EXPECT_EQ(lobby.state(), lobby_state::IDLE);
  }

  TEST_F(steam_tests, join_requested_triggers_auto_join) {
    steam_lobby lobby;
    uint64_t connect_host = 0;
    uint64_t requested_lobby = 0;
    lobby.set_hooks({
      .ready_to_connect = [&](uint64_t host_id) { connect_host = host_id; },
      /// the glue's auto-join, minus the driver
      .join_requested = [&](uint64_t lobby_id) {
        requested_lobby = lobby_id;
        lobby.join(lobby_id);
      },
    });

    lobby.on_join_requested(7);
    EXPECT_EQ(requested_lobby, 7u);
    EXPECT_EQ(lobby.state(), lobby_state::JOINING);

    lobby.on_lobby_entered(7, 99, true);
    EXPECT_EQ(lobby.state(), lobby_state::IN_LOBBY);
    EXPECT_EQ(lobby.lobby_id(), 7u);
    EXPECT_EQ(connect_host, 99u);
  }

  TEST_F(steam_tests, failed_lobby_entry_returns_to_idle_without_connecting) {
    steam_lobby lobby;
    uint64_t connect_host = 0;
    lobby.set_hooks({ .ready_to_connect = [&](uint64_t host_id) { connect_host = host_id; } });

    ASSERT_TRUE(lobby.join(7));
    lobby.on_lobby_entered(7, 0, false);
    EXPECT_EQ(lobby.state(), lobby_state::IDLE);
    EXPECT_EQ(connect_host, 0u);
  }

  /// ------------------------------------------------------------ address routing

  namespace {

    class recording_stub_transport final : public link_transport {
     public:
      std::string_view name() const override { return "steam"; }
      bool is_stream() const override { return false; }
      link_caps conn_caps(natural_t) const override { return { .reliable = true, .ordered = true, .max_frame_size = 0 }; }
      void bind(callbacks) override {}
      natural_t dial(const net_address& remote) override {
        dials.push_back(remote);
        return 0;  // refuse: routing is the assertion, not establishment
      }
      natural_t listen(const net_address& bind_addr) override {
        listens.push_back(bind_addr);
        return 0;
      }
      void tx(natural_t, std::span<const uint8_t>) override {}
      void close(natural_t) override {}

      ostd::vector<net_address> dials;
      ostd::vector<net_address> listens;
    };

  }  // namespace

  TEST_F(steam_tests, steam_address_kinds_route_to_the_steam_transport) {
    mesh_sim_fixture sim(1);
    recording_stub_transport stub;
    sim.mesh().register_transport(stub);

    sim.base_actor(1).open_link({ .addressing = net_address::kind::STEAM_PEER, .id = 77 });
    ASSERT_EQ(stub.dials.size(), 1u);
    EXPECT_EQ(stub.dials[0].addressing, net_address::kind::STEAM_PEER);
    EXPECT_EQ(stub.dials[0].id, 77u);

    sim.base_actor(1).open_link({ .addressing = net_address::kind::STEAM_LOBBY, .id = 12 });
    EXPECT_EQ(stub.dials.size(), 2u);

    sim.base_actor(1).open_listener({ .addressing = net_address::kind::STEAM_PEER });
    EXPECT_EQ(stub.listens.size(), 1u);
  }

  TEST_F(steam_tests, steam_addresses_without_a_steam_transport_refuse_cleanly) {
    mesh_sim_fixture sim(1);
    EXPECT_EQ(sim.base_actor(1).open_link({ .addressing = net_address::kind::STEAM_PEER, .id = 77 }), 0u);
  }

  /// ------------------------------------------------------------ degradation

  TEST_F(steam_tests, steam_unavailable_is_inert_and_never_fatal) {
    steam_context context;
    EXPECT_EQ(context.state(), steam_state::DISABLED);
    EXPECT_EQ(context.local_steam_id(), 0u);

    /// no appid file is written for id 0; on CI (no client) this is UNAVAILABLE,
    ///  on a dev box with steam running it may be READY — both are contract-legal
    const steam_state result = context.initialize(0);
    EXPECT_EQ(result, context.state());
    EXPECT_NE(result, steam_state::DISABLED);
    if (result == steam_state::READY) {
      EXPECT_NE(context.local_steam_id(), 0u);
    } else {
      EXPECT_EQ(context.local_steam_id(), 0u);
      EXPECT_TRUE(context.persona_name().empty());
    }

    context.pump();
    context.shutdown();
    EXPECT_EQ(context.state(), steam_state::DISABLED);
  }

}  // namespace other
