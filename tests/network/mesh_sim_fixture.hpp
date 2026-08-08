/**
 * \file network/mesh_sim_fixture.hpp
 **/
#ifndef OTHER_TESTS_NETWORK_MESH_SIM_FIXTURE_HPP
#define OTHER_TESTS_NETWORK_MESH_SIM_FIXTURE_HPP

#include "core/scope.hpp"

#include "network/memory/memory_fabric.hpp"
#include "peer_mesh/peer_mesh.hpp"
#include "peer_mesh/peer_mesh_actor.hpp"

namespace other {

  struct recorded_frame {
    natural_t via_link = 0;
    node_id src = 0;
    uint16_t net_id = 0;
    ostd::vector<uint8_t> payload;
  };

  struct recorded_down {
    link_record link;
    link_close_reason reason = link_close_reason::NONE;
  };

  class test_actor final : public peer_mesh_actor {
   public:
    std::string_view name() const override { return "test-actor"; }

    void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override {
      frames.push_back({ via.link_id, src, net_id, ostd::vector<uint8_t>(payload.begin(), payload.end()) });
    }
    void on_link_up(const link_record& link) override { ups.push_back(link); }
    void on_link_down(const link_record& link, link_close_reason reason) override { downs.push_back({ link, reason }); }

    ostd::vector<recorded_frame> frames;
    ostd::vector<link_record> ups;
    ostd::vector<recorded_down> downs;
  };

  /// the simulation shape: one process, one fabric, one mesh, N actors (ids 1..N —
  ///  the programmatic layout hook). deterministic virtual clock via step()
  struct mesh_sim_fixture {
    explicit mesh_sim_fixture(size_t actors, uint64_t seed = 1, const peer_mesh_config& cfg = {})
        : fabric(seed), port(fabric), sim_mesh("sim", cfg) {
      sim_mesh.register_transport(port);
      for (size_t i = 1; i <= actors; ++i) {
        sim_mesh.spawn_actor(make_scope<test_actor>(), static_cast<node_id>(i));
      }
    }

    peer_mesh& mesh() { return sim_mesh; }

    /// works for any spawned actor type; session tests spawn their own actors
    peer_mesh_actor& base_actor(size_t node) {
      peer_mesh_actor* a = sim_mesh.actor(static_cast<node_id>(node));
      OTHER_ASSERT(a != nullptr, "fixture: no actor {}", node);
      return *a;
    }

    test_actor& actor(size_t node) {
      return static_cast<test_actor&>(base_actor(node));
    }

    /// b listens on a fresh endpoint, a dials it. returns a's link id; run step()
    ///  a few times to complete the handshake
    natural_t link(size_t a, size_t b, const link_profile& ab = {}, const link_profile& ba = {}, bool datagram = false) {
      const uint64_t endpoint_id = next_endpoint++;
      fabric.configure_endpoint(endpoint_id, datagram);

      const natural_t listener = base_actor(b).open_listener(net_address::memory_endpoint(endpoint_id));
      OTHER_ASSERT(listener != 0, "fixture: listen failed for endpoint {}", endpoint_id);

      const natural_t link_id = base_actor(a).open_link(net_address::memory_endpoint(endpoint_id));
      OTHER_ASSERT(link_id != 0, "fixture: dial failed for endpoint {}", endpoint_id);

      const link_record* record = sim_mesh.net().link(link_id);
      OTHER_ASSERT(record != nullptr, "fixture: dialed link {} missing", link_id);
      fabric.set_profile(record->connection_id, ab, ba);
      return link_id;
    }

    void step(microseconds dt = microseconds{ 1000 }, size_t ticks = 1) {
      for (size_t i = 0; i < ticks; ++i) {
        now += dt;
        sim_mesh.tick(now);
        fabric.tick(now);
      }
    }

    /// steps until the predicate holds or the budget runs out; returns success
    template <typename Pred>
    bool step_until(Pred&& done, size_t max_ticks = 64, microseconds dt = microseconds{ 1000 }) {
      for (size_t i = 0; i < max_ticks; ++i) {
        if (done()) {
          return true;
        }
        step(dt);
      }
      return done();
    }

    microseconds now{ 0 };
    uint64_t next_endpoint = 1;

    memory_fabric fabric;
    fabric_port port;
    peer_mesh sim_mesh;
  };

}  // namespace other

#endif  // OTHER_TESTS_NETWORK_MESH_SIM_FIXTURE_HPP
