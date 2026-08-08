/**
 * \file network/scene_mode_tests.cpp
 *  M6: Mode 1 (authored) + the [Replicated] lane + the MANET sample — the arc's
 *  exit proofs. steam rows excluded (04 §10 live anchors).
 **/
#include <algorithm>

#include <gtest/gtest.h>

#include "object/network_component.hpp"
#include "object/network_settings_component.hpp"
#include "object/transform.hpp"
#include "scene/scene.hpp"
#include "serialization/scene_serializer.hpp"

#include "network/session/authored_session.hpp"
#include "network/session/network_session.hpp"
#include "network/session/replication.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "network/manet_scenario.hpp"
#include "network/mesh_sim_fixture.hpp"
#include "other_test.hpp"

namespace other {

  class scene_mode_tests : public other_test {
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

    /// M4's harness shape: host + client seats with scenes and replication
    struct mode_sim {
      mesh_sim_fixture sim;
      scene host_scene{ "host-world" };
      scene client_scene{ "client-world" };
      network_session* host_session = nullptr;
      network_session* client_session = nullptr;
      scope<replication> host_repl;
      scope<replication> client_repl;
      uint64_t endpoint = 0;

      mode_sim()
          : sim(0, 1, fast_cfg()) {
        host_session = &spawn(1, "host");
        client_session = &spawn(2, "client");
        host_repl = make_scope<replication>(*host_session, [this] { return &host_scene; });
        client_repl = make_scope<replication>(*client_session, [this] { return &client_scene; });
        host_session->set_observer([this](session_event ev, uint16_t arg) { host_repl->on_session_event(ev, arg); });
        client_session->set_observer([this](session_event ev, uint16_t arg) { client_repl->on_session_event(ev, arg); });
        endpoint = sim.next_endpoint++;
        sim.fabric.configure_endpoint(endpoint, false);
      }

      ~mode_sim() {
        host_session->set_observer(nullptr);
        client_session->set_observer(nullptr);
      }

      network_session& spawn(node_id node, std::string name) {
        return static_cast<network_session&>(sim.mesh().spawn_actor(make_scope<network_session>(fast_scfg(std::move(name))), node));
      }

      void step(size_t ticks = 1, microseconds dt = microseconds{ 5'000 }) {
        for (size_t i = 0; i < ticks; ++i) {
          sim.step(dt);
          host_repl->tick(sim.now);
          client_repl->tick(sim.now);
        }
      }

      template <typename Pred>
      bool step_until(Pred&& done, size_t max_ticks = 512) {
        for (size_t i = 0; i < max_ticks; ++i) {
          if (done()) {
            return true;
          }
          step();
        }
        return done();
      }
    };

  }  // namespace

  /// ------------------------------------------------------------ codec rows

  TEST_F(scene_mode_tests, network_settings_codec_round_trips) {
    scene source{ "source" };
    scene_object& holder = source.create_object("session");
    network_settings_component& settings = source.add_component<network_settings_component>(&holder);
    settings.session_mode = "host";
    settings.transport = "tcp";
    settings.port = 43999;
    settings.max_peers = 4;
    settings.spawn_template = "player-template";

    const serialization::scene_document doc = serialization::capture_scene(source, serialization::default_codec_services());
    scene target{ "target" };
    serialization::instantiate_scene(target, doc, serialization::default_codec_services());

    const network_settings_component* round = authored_session::find_settings(target);
    ASSERT_NE(round, nullptr);
    EXPECT_EQ(round->session_mode, "host");
    EXPECT_EQ(round->transport, "tcp");
    EXPECT_EQ(round->port, 43999);
    EXPECT_EQ(round->max_peers, 4);
    EXPECT_EQ(round->spawn_template, "player-template");
  }

  TEST_F(scene_mode_tests, network_component_codec_round_trips) {
    scene source{ "source" };
    scene_object& pawn = source.create_object("pawn");
    network_component& net = source.add_component<network_component>(&pawn);
    net.owner_peer = 3;
    net.replicate_transform = false;
    net.despawn_on_owner_leave = false;

    const serialization::scene_document doc = serialization::capture_scene(source, serialization::default_codec_services());
    scene target{ "target" };
    serialization::instantiate_scene(target, doc, serialization::default_codec_services());

    const network_component* round = target.get_component<network_component>(target.find_object("pawn"));
    ASSERT_NE(round, nullptr);
    EXPECT_EQ(round->owner_peer, 3u);
    EXPECT_FALSE(round->replicate_transform);
    EXPECT_FALSE(round->despawn_on_owner_leave);
  }

  /// ------------------------------------------------------------ authored mode

  TEST_F(scene_mode_tests, authored_host_scene_starts_session_on_play) {
    mode_sim fx;
    scene_object& holder = fx.host_scene.create_object("session");
    network_settings_component& settings = fx.host_scene.add_component<network_settings_component>(&holder);
    settings.session_mode = "host";
    settings.max_peers = 2;
    settings.port = 43999;

    const network_settings_component* found = authored_session::find_settings(fx.host_scene);
    ASSERT_NE(found, nullptr);
    /// the sim engine listens on the memory fabric instead of sockets
    uint16_t listened_port = 0;
    const authored_session::defaults engine{
      .resolve_listen = [&](uint16_t port) {
        listened_port = port;
        return net_address::memory_endpoint(fx.endpoint);
      },
    };
    ASSERT_TRUE(authored_session::start(*fx.host_session, *found, engine));
    EXPECT_TRUE(fx.host_session->is_host());
    EXPECT_EQ(listened_port, 43999);

    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }));
    /// authored max_peers reached the session: the room is now full (host + 1)
    EXPECT_EQ(fx.host_session->peers().size(), 2u);
  }

  TEST_F(scene_mode_tests, authored_join_scene_dials_on_play) {
    mode_sim fx;
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));

    scene_object& holder = fx.client_scene.create_object("session");
    network_settings_component& settings = fx.client_scene.add_component<network_settings_component>(&holder);
    settings.session_mode = "join";
    settings.address = "10.0.0.1:43999";  // resolver decides what this means

    const network_settings_component* found = authored_session::find_settings(fx.client_scene);
    ASSERT_NE(found, nullptr);
    std::string dialed_address;
    uint16_t dialed_port = 0;
    const authored_session::defaults engine{
      .resolve_address = [&](std::string_view address, uint16_t port) {
        dialed_address = std::string(address);
        dialed_port = port;
        return net_address::memory_endpoint(fx.endpoint);
      },
    };
    ASSERT_TRUE(authored_session::start(*fx.client_session, *found, engine));
    EXPECT_EQ(dialed_address, "10.0.0.1");
    EXPECT_EQ(dialed_port, 43999);
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }));

    /// off mode starts nothing
    network_settings_component off;
    EXPECT_FALSE(authored_session::start(*fx.client_session, off, engine));
  }

  TEST_F(scene_mode_tests, peer_join_clones_spawn_template_with_owner) {
    mode_sim fx;
    /// the disabled template, network component authored for the clones to inherit
    scene_object& template_root = fx.host_scene.create_object("player-template");
    template_root.visible = false;
    fx.host_scene.get_component<transform>(&template_root)->local_position = { 0.f, 3.f, 0.f };
    scene_object& hat = fx.host_scene.create_object("hat", &template_root);
    fx.host_scene.get_component<transform>(&hat)->local_position = { 0.f, 1.f, 0.f };
    network_component& net = fx.host_scene.add_component<network_component>(&template_root);
    net.despawn_on_owner_leave = true;

    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }));
    const uint16_t peer = fx.client_session->local_peer_id();

    const natural_t clone_id = authored_session::spawn_template(fx.host_scene, *fx.host_repl, "player-template", peer);
    ASSERT_NE(clone_id, 0u);

    scene_object* clone = fx.host_scene.find_object(clone_id);
    EXPECT_EQ(clone->name, "player-template-p" + std::to_string(peer));
    EXPECT_TRUE(clone->visible);
    EXPECT_EQ(fx.host_scene.get_component<network_component>(clone)->owner_peer, peer);
    EXPECT_TRUE(fx.host_repl->is_mine(clone_id) == (peer == fx.host_session->local_peer_id()));

    /// the clone replicates to the joiner (with subtree + ownership)
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object(clone->name) != nullptr; }));
    scene_object* replica = fx.client_scene.find_object(clone->name);
    const opt<natural_t> net_id = fx.host_scene.network().net_of(clone_id);
    ASSERT_TRUE(net_id.has_value());
    EXPECT_EQ(fx.client_scene.network().object_of(*net_id), opt<natural_t>{ replica->id });

    /// owner leave despawns the owned clone through the M4 path
    fx.client_session->leave();
    ASSERT_TRUE(fx.step_until([&] { return fx.host_scene.find_object(clone_id) == nullptr; }));
  }

  TEST_F(scene_mode_tests, template_clone_matches_subtree_capture) {
    scene s{ "world" };
    scene_object& template_root = s.create_object("crate-template");
    template_root.visible = false;
    s.get_component<transform>(&template_root)->local_position = { 2.f, 0.f, 1.f };
    scene_object& lid = s.create_object("lid", &template_root);
    s.get_component<transform>(&lid)->local_position = { 0.f, 0.5f, 0.f };
    s.create_object("latch", &lid);

    const serialization::scene_document original =
      serialization::capture_object_subtree(s, template_root.id, serialization::default_codec_services());
    ostd::map<natural_t, natural_t> remap;
    serialization::instantiate_subtree(s, original, nullptr, serialization::default_codec_services(), &remap);
    const serialization::scene_document clone =
      serialization::capture_object_subtree(s, remap[template_root.id], serialization::default_codec_services());

    /// field-wise compare, runtime ids excluded: same shape, names, components
    ASSERT_EQ(clone.objects.size(), original.objects.size());
    for (size_t i = 0; i < original.objects.size(); ++i) {
      EXPECT_EQ(clone.objects[i].name, original.objects[i].name);
      EXPECT_EQ(clone.objects[i].visible, original.objects[i].visible);
      EXPECT_EQ(clone.objects[i].parent_file_id == 0, original.objects[i].parent_file_id == 0);
      ASSERT_EQ(clone.objects[i].components.size(), original.objects[i].components.size());
      for (size_t c = 0; c < original.objects[i].components.size(); ++c) {
        EXPECT_EQ(clone.objects[i].components[c].key_hash, original.objects[i].components[c].key_hash);
        EXPECT_EQ(clone.objects[i].components[c].payload, original.objects[i].components[c].payload);
      }
    }
  }

  /// ------------------------------------------------------------ [Replicated] lane

  TEST_F(scene_mode_tests, replicated_fields_sweep_round_trips) {
    mode_sim fx;
    ASSERT_TRUE(fx.host_session->host(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.client_session->join(net_address::memory_endpoint(fx.endpoint)));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_session->in_session(); }));

    scene_object& turret = fx.host_scene.create_object("turret");
    ASSERT_TRUE(fx.host_repl->spawn_object(turret.id));
    ASSERT_TRUE(fx.step_until([&] { return fx.client_scene.find_object("turret") != nullptr; }));
    const natural_t client_object = fx.client_scene.find_object("turret")->id;

    /// fake managed ends: the host collector reports one dirty blob, the client
    ///  applier records what arrived (the C# codec is symmetric managed-side)
    ostd::vector<uint8_t> to_ship{ 0xAA, 0xBB };
    size_t collects = 0;
    fx.host_repl->set_script_field_hooks(
      [&](natural_t object_id) -> ostd::vector<uint8_t> {
        collects++;
        EXPECT_EQ(object_id, turret.id);
        ostd::vector<uint8_t> out = to_ship;
        to_ship.clear();  // dirty once, clean after
        return out;
      },
      nullptr);
    ostd::vector<std::pair<natural_t, ostd::vector<uint8_t>>> applied;
    fx.client_repl->set_script_field_hooks(nullptr, [&](natural_t object_id, std::span<const uint8_t> payload) {
      applied.push_back({ object_id, ostd::vector<uint8_t>(payload.begin(), payload.end()) });
    });

    ASSERT_TRUE(fx.step_until([&] { return !applied.empty(); }));
    EXPECT_EQ(applied[0].first, client_object);
    EXPECT_EQ(applied[0].second, (ostd::vector<uint8_t>{ 0xAA, 0xBB }));

    /// clean sweeps ship nothing
    const size_t applied_count = applied.size();
    fx.step(32);
    EXPECT_EQ(applied.size(), applied_count);
    EXPECT_GT(collects, 1u);
  }

  TEST_F(scene_mode_tests, no_networking_authored_costs_nothing) {
    scene s{ "plain" };
    s.create_object("just-a-cube");

    /// no session started, nothing authored: the resting state is NONE — no
    ///  registries, no sweeps, no interp, no ticks consumed
    network_session unspawned;
    replication repl(unspawned, [&] { return &s; });
    for (int i = 0; i < 32; ++i) {
      repl.tick(microseconds{ i * 1000 });
    }
    EXPECT_EQ(s.network().role, replication_role::NONE);
    EXPECT_TRUE(s.network().entries().empty());
    EXPECT_EQ(repl.host_tick(), 0u);
    EXPECT_EQ(authored_session::find_settings(s), nullptr);
  }

  /// ------------------------------------------------------------ the MANET sample

  namespace {

    struct manet_run {
      natural_t rounds_at_first_partition = 0;
      natural_t rounds_at_end = 0;
      natural_t first_partition_tick = 0;
      natural_t first_heal_tick = 0;
      ostd::vector<natural_t> arrivals;
      ostd::vector<natural_t> forwards;
    };

    manet_run run_manet(uint64_t seed, size_t ticks) {
      /// a connected 8-node disc cluster carries ~2 link records per pair (both
      ///  seats live on the one sim mesh) — the default 32 cap is for sessions
      peer_mesh_config mesh_cfg;
      mesh_cfg.max_links = 128;
      peer_mesh mesh("manet-sample", mesh_cfg);
      manet_config cfg;
      cfg.seed = seed;
      manet_director& director = static_cast<manet_director&>(mesh.spawn_actor(make_scope<manet_director>(cfg), 999));

      manet_run run;
      const node_id wanderer = cfg.node_count;
      bool was_connected = false;
      microseconds now{ 0 };
      for (size_t i = 0; i < ticks; ++i) {
        now += microseconds{ 50'000 };
        mesh.tick(now);

        const bool connected = director.member_link_count(wanderer) > 0;
        if (was_connected && !connected && run.first_partition_tick == 0) {
          run.first_partition_tick = director.ticks();
          run.rounds_at_first_partition = director.member(1).token_arrivals;
        }
        if (!was_connected && connected && run.first_partition_tick != 0 && run.first_heal_tick == 0) {
          run.first_heal_tick = director.ticks();
        }
        was_connected = connected;
      }

      run.rounds_at_end = director.member(1).token_arrivals;
      for (size_t i = 1; i <= cfg.node_count; ++i) {
        run.arrivals.push_back(director.member(i).token_arrivals);
        run.forwards.push_back(director.member(i).token_forwards);
      }
      return run;
    }

  }  // namespace

  TEST_F(scene_mode_tests, manet_sample_scenario_partitions_and_recovers_deterministically) {
    const manet_run first = run_manet(7, 3000);

    /// mobility partitions the wanderer and heals it; the token keeps circulating
    ///  whenever a path exists
    EXPECT_GT(first.first_partition_tick, 0u);
    EXPECT_GT(first.first_heal_tick, first.first_partition_tick);
    EXPECT_GT(first.rounds_at_first_partition, 1u);
    EXPECT_GT(first.rounds_at_end, first.rounds_at_first_partition);

    /// deterministic under a fixed seed: an identical rerun is identical
    const manet_run second = run_manet(7, 3000);
    EXPECT_EQ(second.first_partition_tick, first.first_partition_tick);
    EXPECT_EQ(second.first_heal_tick, first.first_heal_tick);
    EXPECT_EQ(second.arrivals, first.arrivals);
    EXPECT_EQ(second.forwards, first.forwards);
  }

}  // namespace other
