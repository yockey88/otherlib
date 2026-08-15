/**
 * \file network/manet_scenario.hpp
 *
 * shipped "ideal" MANET sample (Mode-4 exemplar): mesh + N actors + director driving a toy
 *  disc radio/mobility/routing/token-ring; not real propagation (analyzer project's job)
 **/
#ifndef OTHER_TESTS_NETWORK_MANET_SCENARIO_HPP
#define OTHER_TESTS_NETWORK_MANET_SCENARIO_HPP

#include <cmath>

#include "core/scope.hpp"

#include "network/memory/memory_transport_provider.hpp"
#include "peer_mesh/peer_mesh.hpp"
#include "peer_mesh/peer_actor.hpp"

namespace other {

  struct manet_config {
    size_t node_count = 8;      // members, node ids 1..N (programmatic layout, D13)
    float cluster_extent = 50.f;  // waypoints land in [25, 25+extent]^2
    float radio_range = 38.f;
    float speed = 1.5f;  // units per director tick
    uint64_t seed = 1;
  };

  /// application net ids + envelope for the sample's own wire format (D22: the module
  ///  carries whatever bytes the actors exchange). multi-hop is member behavior: every
  ///  frame opens with [dst u8][origin u8][hops u8] and members relay by route table
  constexpr uint16_t kManetTokenId = 0x100;
  constexpr uint16_t kManetPingId = 0x101;
  constexpr size_t kManetEnvelope = 3;

  /// a scenario member: forwards the token around the ring whenever a path exists,
  ///  pings its ring neighbor for background traffic, relays frames for its peers
  class manet_member final : public peer_actor {
   public:
    manet_member(size_t member_count)
        : member_count(member_count) {}

    std::string_view name() const override { return "manet-member"; }

    void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override {
      if (payload.size() < kManetEnvelope) {
        return;
      }
      const node_id dst = payload[0];
      const uint8_t hops = payload[2];

      if (dst != id()) {
        /// someone else's frame at this seat: relay toward dst, or drop when the
        ///  route table has no path (partitioned) or the hop budget ran out
        if (hops > 0) {
          if (auto hop = next_hops.find(dst); hop != next_hops.end()) {
            ostd::vector<uint8_t> onward(payload.begin(), payload.end());
            onward[2] = static_cast<uint8_t>(hops - 1);
            if (send(hop->second, net_id, onward)) {
              relays++;
              return;
            }
          }
        }
        relay_drops++;
        return;
      }

      if (net_id == kManetTokenId) {
        token_arrivals++;
        has_token = true;
      } else if (net_id == kManetPingId) {
        pings_seen++;
      }
    }

    void tick(microseconds now, double dt) override {
      if (has_token) {
        if (routed_send((id() % member_count) + 1, kManetTokenId)) {
          has_token = false;
          token_forwards++;
        }
        /// no route (partitioned): keep the token, retry next tick
      }
      if (++ping_phase % 16 == 0) {
        routed_send((id() % member_count) + 1, kManetPingId);
      }
    }

    /// origin-side send along the route table; single-hop when dst is adjacent
    bool routed_send(node_id dst, uint16_t net_id) {
      auto hop = next_hops.find(dst);
      if (hop == next_hops.end()) {
        return false;
      }
      const uint8_t envelope[kManetEnvelope] = {
        static_cast<uint8_t>(dst), static_cast<uint8_t>(id()), static_cast<uint8_t>(member_count)
      };
      return send(hop->second, net_id, envelope);
    }

    /// route table: dst -> adjacent next hop, rebuilt by the director each tick
    ostd::map<node_id, node_id> next_hops;

    /// mobility state, driven by the director
    float x = 0.f, y = 0.f;
    float waypoint_x = 0.f, waypoint_y = 0.f;

    bool has_token = false;
    natural_t token_arrivals = 0;
    natural_t token_forwards = 0;
    natural_t pings_seen = 0;
    natural_t relays = 0;
    natural_t relay_drops = 0;
    size_t ping_phase = 0;

   private:
    size_t member_count;
  };

  /// the director: spawns the members, owns the simulated medium, and each tick
  ///  applies mobility + the disc radio model from every member's seat
  class manet_director final : public peer_actor {
   public:
    explicit manet_director(const manet_config& scenario_cfg = {})
        : cfg(scenario_cfg), fabric(scenario_cfg.seed) {}

    std::string_view name() const override { return "manet-sample"; }
    void on_frame(const link_record&, node_id, uint16_t, std::span<const uint8_t>) override {}

    void tick(microseconds now, double dt) override {
      if (!bootstrapped) {
        bootstrap();
      }
      move_members();
      apply_disc_radio();
      rebuild_routes();
      fabric.tick(now);
      scenario_ticks++;
    }

    manet_member& member(node_id node) { return *static_cast<manet_member*>(mesh().actor(node)); }
    natural_t member_link_count(node_id node) {
      natural_t count = 0;
      for (const link_record& link : mesh().links()) {
        if (link.local == node && link.state == link_state::UP) {
          count++;
        }
      }
      return count;
    }
    natural_t ticks() const { return scenario_ticks; }

   private:
    manet_config cfg;
    memory_transport_provider fabric;
    bool bootstrapped = false;
    natural_t scenario_ticks = 0;
    uint64_t prng_state = 0;
    /// pair (a<b) -> a's dialed link id; a link mid-handshake has no remote yet,
    ///  so presence here (with the link still alive) is the "already linked" check
    ostd::map<std::pair<node_id, node_id>, natural_t> pair_links;

    /// splitmix64 — deterministic waypoints under a fixed seed
    uint64_t next_random() {
      prng_state += 0x9E3779B97F4A7C15ull;
      uint64_t z = prng_state;
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
      return z ^ (z >> 31);
    }
    float random_in_cluster() {
      return 25.f + static_cast<float>(next_random() % 1000) * (cfg.cluster_extent / 1000.f);
    }

    void bootstrap() {
      bootstrapped = true;
      prng_state = cfg.seed;
      mesh().attach_provider(fabric);

      for (size_t i = 1; i <= cfg.node_count; ++i) {
        fabric.configure_endpoint(i, false);
        manet_member& node = static_cast<manet_member&>(
          mesh().add_secondary(make_scope<manet_member>(cfg.node_count), static_cast<node_id>(i)));
        node.open_listener(net_address::memory_endpoint(i));
        node.x = random_in_cluster();
        node.y = random_in_cluster();
        node.waypoint_x = random_in_cluster();
        node.waypoint_y = random_in_cluster();
      }
      /// the wanderer: waypoints alternate between the cluster and far outside
      ///  radio range, guaranteeing partition + re-join cycles
      manet_member& wanderer = member(static_cast<node_id>(cfg.node_count));
      wanderer.waypoint_x = 300.f;
      wanderer.waypoint_y = 300.f;

      member(1).has_token = true;
      member(1).token_arrivals = 1;
    }

    void move_members() {
      for (size_t i = 1; i <= cfg.node_count; ++i) {
        manet_member& node = member(static_cast<node_id>(i));
        const float dx = node.waypoint_x - node.x;
        const float dy = node.waypoint_y - node.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < cfg.speed) {
          node.x = node.waypoint_x;
          node.y = node.waypoint_y;
          if (i == cfg.node_count) {
            /// wanderer: bounce between the far point and the cluster
            const bool away = node.waypoint_x > 200.f;
            node.waypoint_x = away ? random_in_cluster() : 300.f;
            node.waypoint_y = away ? random_in_cluster() : 300.f;
          } else {
            node.waypoint_x = random_in_cluster();
            node.waypoint_y = random_in_cluster();
          }
          continue;
        }
        node.x += dx / dist * cfg.speed;
        node.y += dy / dist * cfg.speed;
      }
    }

    float distance(node_id a, node_id b) {
      const manet_member& ma = member(a);
      const manet_member& mb = member(b);
      const float dx = ma.x - mb.x;
      const float dy = ma.y - mb.y;
      return std::sqrt(dx * dx + dy * dy);
    }

    void apply_disc_radio() {
      for (size_t a = 1; a <= cfg.node_count; ++a) {
        for (size_t b = a + 1; b <= cfg.node_count; ++b) {
          const auto key = std::pair{ static_cast<node_id>(a), static_cast<node_id>(b) };
          const float dist = distance(a, b);

          const link_record* link = nullptr;
          if (auto itr = pair_links.find(key); itr != pair_links.end()) {
            link = mesh().link(itr->second);
            if (link == nullptr) {
              pair_links.erase(itr);  // died (timeout/close); redial when in range
            }
          }

          if (link == nullptr) {
            if (dist < cfg.radio_range) {
              if (const natural_t link_id = member(a).open_link(net_address::memory_endpoint(b)); link_id != 0) {
                pair_links[key] = link_id;
              }
            }
          } else if (dist > cfg.radio_range * 1.15f) {
            /// hysteresis so range-edge pairs don't flap
            member(a).close_link(link->link_id, link_close_reason::SHUTDOWN);
            pair_links.erase(key);
          } else if (link->state == link_state::UP) {
            /// latency scales with distance — the toy propagation model
            const link_profile profile{ .latency = microseconds{ 1000 + static_cast<int64_t>(dist * 40.f) } };
            fabric.set_profile(link->connection_id, profile, profile);
          }
        }
      }
    }

    /// BFS over the in-range UP-link adjacency; every member gets dst -> first-hop
    ///  rows — the director is the sample's routing protocol, the mesh carries frames
    void rebuild_routes() {
      const size_t count = cfg.node_count;
      for (size_t src = 1; src <= count; ++src) {
        member(static_cast<node_id>(src)).next_hops.clear();
      }
      for (size_t src = 1; src <= count; ++src) {
        /// BFS from src; parent[] reconstructs first hops
        ostd::vector<node_id> parent(count + 1, 0);
        ostd::vector<node_id> queue{ static_cast<node_id>(src) };
        parent[src] = static_cast<node_id>(src);
        for (size_t head = 0; head < queue.size(); ++head) {
          const node_id at = queue[head];
          for (size_t next = 1; next <= count; ++next) {
            if (parent[next] != 0) {
              continue;
            }
            const link_record* link = mesh().link_between(at, next);
            if (link == nullptr || link->state != link_state::UP) {
              continue;
            }
            parent[next] = at;
            queue.push_back(static_cast<node_id>(next));
          }
        }

        for (size_t dst = 1; dst <= count; ++dst) {
          if (dst == src || parent[dst] == 0) {
            continue;
          }
          /// walk back from dst to find src's first hop
          node_id hop = static_cast<node_id>(dst);
          while (parent[hop] != static_cast<node_id>(src)) {
            hop = parent[hop];
          }
          if (mesh().link_between(static_cast<node_id>(src), hop) != nullptr) {
            member(static_cast<node_id>(src)).next_hops[static_cast<node_id>(dst)] = hop;
          }
        }
      }
    }
  };

}  // namespace other

#endif  // OTHER_TESTS_NETWORK_MANET_SCENARIO_HPP
