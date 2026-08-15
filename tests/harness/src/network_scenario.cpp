/**
 * \file tests/harness/src/network_scenario.cpp
 **/
#include "network_scenario.hpp"

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/logger.hpp"

#include "peer_mesh/peer_actor.hpp"

namespace other {

  namespace {

    constexpr uint16_t kTelemetryNear = 10;
    constexpr uint16_t kTelemetryFar = 11;
    constexpr uint16_t kDatagramLane = 12;

    /// payload: [u64 send_us][u32 round], little-endian, padded to 64 B of traffic
    constexpr size_t kTelemetrySize = 64;
    /// telemetry lanes open with the scenario's own routing envelope — relaying is
    ///  actor behavior over director-fed route tables, not a mesh service
    constexpr size_t kEnvelopeSize = 17;  // [u64 origin][u64 dst][u8 hops]

    uint64_t envelope_u64(std::span<const uint8_t> bytes, size_t at) {
      uint64_t value = 0;
      for (size_t i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(bytes[at + i]) << (i * 8);
      }
      return value;
    }

    void write_envelope(uint8_t* out, node_id origin, node_id dst, uint8_t hops) {
      for (size_t i = 0; i < 8; ++i) {
        out[i] = static_cast<uint8_t>(origin >> (i * 8));
        out[8 + i] = static_cast<uint8_t>(dst >> (i * 8));
      }
      out[16] = hops;
    }

    ostd::vector<uint8_t> telemetry_payload(microseconds now, uint32_t round) {
      ostd::vector<uint8_t> payload(kTelemetrySize, 0);
      const uint64_t us = static_cast<uint64_t>(now.count());
      for (size_t i = 0; i < 8; ++i) {
        payload[i] = static_cast<uint8_t>(us >> (i * 8));
      }
      for (size_t i = 0; i < 4; ++i) {
        payload[8 + i] = static_cast<uint8_t>(round >> (i * 8));
      }
      return payload;
    }

    int64_t telemetry_send_us(std::span<const uint8_t> payload) {
      if (payload.size() < 8) {
        return -1;
      }
      uint64_t us = 0;
      for (size_t i = 0; i < 8; ++i) {
        us |= static_cast<uint64_t>(payload[i]) << (i * 8);
      }
      return static_cast<int64_t>(us);
    }

    /// deterministic per-scenario prng for topology profiles (splitmix64 core)
    struct topology_rng {
      uint64_t state = 0;
      uint64_t next() {
        state += 0x9E3779B97F4A7C15ull;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
      }
      int64_t range(int64_t lo, int64_t hi) {
        if (hi <= lo) {
          return lo;
        }
        return lo + static_cast<int64_t>(next() % static_cast<uint64_t>(hi - lo));
      }
    };

  }  // namespace

  /// a scenario member: relays enveloped telemetry along its route table, records
  ///  deliveries + end-to-end latency into the scenario at the destination seat
  class scenario_actor final : public peer_actor {
   public:
    explicit scenario_actor(network_scenario* owner)
        : owner(owner) {}

    std::string_view name() const override { return "scenario-actor"; }

    void on_frame(const link_record& via, node_id src, uint16_t net_id, std::span<const uint8_t> payload) override {
      if (net_id == kDatagramLane) {
        owner->record_delivery(src, id(), -1, net_id);
        return;
      }
      if (payload.size() < kEnvelopeSize) {
        return;
      }
      const node_id origin = envelope_u64(payload, 0);
      const node_id dst = envelope_u64(payload, 8);
      const uint8_t hops = payload[16];

      if (dst != id()) {
        if (hops > 0) {
          if (auto hop = next_hops.find(dst); hop != next_hops.end()) {
            ostd::vector<uint8_t> onward(payload.begin(), payload.end());
            onward[16] = static_cast<uint8_t>(hops - 1);
            if (send(hop->second, net_id, onward)) {
              return;
            }
          }
        }
        owner->relay_drops++;
        return;
      }

      const int64_t sent_us = telemetry_send_us(payload.subspan(kEnvelopeSize));
      const int64_t latency_us = sent_us >= 0 ? owner->now.count() - sent_us : -1;
      owner->record_delivery(origin, id(), latency_us, net_id);
    }

    /// origin-side send along the route table; false (counted by the caller) when
    ///  the table has no path — the partition's refusal signal
    bool send_routed(node_id dst, uint16_t net_id, std::span<const uint8_t> telemetry, uint8_t hops) {
      auto hop = next_hops.find(dst);
      if (hop == next_hops.end()) {
        return false;
      }
      ostd::vector<uint8_t> payload(kEnvelopeSize + telemetry.size());
      write_envelope(payload.data(), id(), dst, hops);
      std::ranges::copy(telemetry, payload.data() + kEnvelopeSize);
      return send(hop->second, net_id, payload);
    }

    ostd::map<node_id, node_id> next_hops;

   private:
    network_scenario* owner = nullptr;
  };

  void network_scenario::initialize(driver& host, const config_table& config) {
    actor_count = config.get_value<size_t>("harness.network.actors", actor_count);
    seed = config.get_value<size_t>("harness.network.seed", seed);
    tick_us = static_cast<int64_t>(config.get_value<size_t>("harness.network.tick-us", static_cast<size_t>(tick_us)));
    ticks_per_frame = config.get_value<size_t>("harness.network.ticks-per-frame", ticks_per_frame);
    cross_link_stride = config.get_value<size_t>("harness.network.cross-link-stride", cross_link_stride);
    latency_min_us = static_cast<int64_t>(config.get_value<size_t>("harness.network.latency-min-us", static_cast<size_t>(latency_min_us)));
    latency_max_us = static_cast<int64_t>(config.get_value<size_t>("harness.network.latency-max-us", static_cast<size_t>(latency_max_us)));
    jitter_pct = config.get_value<double>("harness.network.jitter-pct", jitter_pct);
    steady_rounds = config.get_value<size_t>("harness.network.steady-rounds", steady_rounds);
    round_interval_ticks = config.get_value<size_t>("harness.network.round-interval-ticks", round_interval_ticks);
    drain_ticks = config.get_value<size_t>("harness.network.drain-ticks", drain_ticks);
    churn_rounds = config.get_value<size_t>("harness.network.churn-rounds", churn_rounds);
    datagram_frames = config.get_value<size_t>("harness.network.datagram-frames", datagram_frames);
    datagram_loss = config.get_value<double>("harness.network.datagram-loss", datagram_loss);
    datagram_loss_tolerance = config.get_value<double>("harness.network.datagram-loss-tolerance", datagram_loss_tolerance);
    max_establish_ticks = config.get_value<size_t>("harness.network.max-establish-ticks", max_establish_ticks);
    min_delivery_ratio = config.get_value<double>("harness.network.min-delivery-ratio", min_delivery_ratio);
    max_e2e_latency_us = static_cast<int64_t>(config.get_value<size_t>("harness.network.max-e2e-latency-us", static_cast<size_t>(max_e2e_latency_us)));
    report_path = config.get_value<std::string>("harness.network.report-path", report_path);

    OTHER_ASSERT(actor_count >= 4, "network scenario needs at least 4 actors (got {})", actor_count);

    peer_mesh_config mesh_cfg;
    mesh_cfg.keepalive_idle = microseconds{ 50'000 };
    mesh_cfg.link_timeout = microseconds{ 60'000'000 };  // partitions are deliberate, not timeouts
    mesh_cfg.max_links = 2 * (actor_count + actor_count / std::max<size_t>(cross_link_stride, 1) + 2);

    fabric = make_scope<memory_transport_provider>(seed);
    mesh = make_scope<peer_mesh>("harness-net", mesh_cfg);
    mesh->attach_provider(*fabric);

    for (size_t i = 1; i <= actor_count; ++i) {
      i == 1 ? mesh->set_primary(make_scope<scenario_actor>(this), 1) : mesh->add_secondary(make_scope<scenario_actor>(this), static_cast<node_id>(i));
    }

    /// chain backbone + cross links every stride; the churn cut severs everything
    ///  crossing the middle, so cross links straddling it are part of the blackout
    const node_id mid = static_cast<node_id>(actor_count / 2);
    for (size_t i = 1; i < actor_count; ++i) {
      plan.push_back({ static_cast<node_id>(i), static_cast<node_id>(i + 1), i == mid });
    }
    if (cross_link_stride >= 2) {
      for (size_t i = 1; i + cross_link_stride <= actor_count; i += cross_link_stride) {
        const node_id a = static_cast<node_id>(i);
        const node_id b = static_cast<node_id>(i + cross_link_stride);
        plan.push_back({ a, b, a <= mid && b > mid });
      }
    }

    CORE_LOG_INFO("[NET-HARNESS] {} actors, {} planned links, seed {}, tick {} us",
                  actor_count, plan.size(), seed, tick_us);
    enter_phase(phase::ESTABLISH);
    open_planned_links();
  }

  void network_scenario::open_planned_links() {
    topology_rng rng{ seed ^ 0xC0FFEEull };
    for (const planned_link& planned : plan) {
      const uint64_t endpoint = (static_cast<uint64_t>(planned.a) << 16) | planned.b;
      fabric->configure_endpoint(endpoint, false);
      peer_actor* listener = mesh->actor(planned.b);
      peer_actor* dialer = mesh->actor(planned.a);
      OTHER_ASSERT(listener != nullptr && dialer != nullptr, "planned link {}-{} has no actors", planned.a, planned.b);
      listener->open_listener(net_address::memory_endpoint(endpoint));
      const natural_t link_id = dialer->open_link(net_address::memory_endpoint(endpoint));
      OTHER_ASSERT(link_id != 0, "planned link {}-{} refused", planned.a, planned.b);

      const int64_t latency = rng.range(latency_min_us, latency_max_us);
      link_profile profile;
      profile.latency = microseconds{ latency };
      profile.jitter = microseconds{ static_cast<int64_t>(latency * jitter_pct) };
      const link_record* record = mesh->link(link_id);
      OTHER_ASSERT(record != nullptr, "planned link {} vanished", link_id);
      fabric->set_profile(record->connection_id, profile, profile);
    }
  }

  bool network_scenario::all_planned_links_up() const {
    for (const planned_link& planned : plan) {
      if (cut_applied && !reopened && planned.crosses_cut) {
        continue;
      }
      if (mesh->link_between(planned.a, planned.b) == nullptr ||
          mesh->link_between(planned.b, planned.a) == nullptr) {
        return false;
      }
    }
    return true;
  }

  void network_scenario::rebuild_routes() {
    /// BFS next-hops over the planned topology (minus an open cut) into each actor's
    ///  route table; the scenario is the routing director, same as the MANET sample
    ostd::map<node_id, ostd::vector<node_id>> adjacency;
    for (const planned_link& planned : plan) {
      if (cut_applied && !reopened && planned.crosses_cut) {
        continue;
      }
      adjacency[planned.a].push_back(planned.b);
      adjacency[planned.b].push_back(planned.a);
    }

    for (size_t src = 1; src <= actor_count; ++src) {
      static_cast<scenario_actor*>(mesh->actor(static_cast<node_id>(src)))->next_hops.clear();
    }
    for (size_t src = 1; src <= actor_count; ++src) {
      const node_id origin = static_cast<node_id>(src);
      scenario_actor* origin_actor = static_cast<scenario_actor*>(mesh->actor(origin));
      ostd::map<node_id, node_id> parent;
      std::deque<node_id> frontier{ origin };
      parent[origin] = origin;
      while (!frontier.empty()) {
        const node_id at = frontier.front();
        frontier.pop_front();
        for (const node_id next : adjacency[at]) {
          if (!parent.contains(next)) {
            parent[next] = at;
            frontier.push_back(next);
          }
        }
      }

      for (size_t dst = 1; dst <= actor_count; ++dst) {
        const node_id target = static_cast<node_id>(dst);
        if (target == origin || !parent.contains(target)) {
          continue;
        }
        node_id hop = target;
        while (parent[hop] != origin) {
          hop = parent[hop];
        }
        if (mesh->link_between(origin, hop) != nullptr) {
          origin_actor->next_hops[target] = hop;
        }
      }
    }
  }

  node_id network_scenario::far_target(node_id from) const {
    return static_cast<node_id>((from + actor_count / 2 - 1) % actor_count + 1);
  }

  void network_scenario::record_delivery(node_id src, node_id dst, int64_t latency_us, uint16_t net_id) {
    if (net_id == kDatagramLane) {
      datagram_delivered++;
      return;
    }

    steady_delivered++;
    if (current_phase == phase::CHURN && reopened && net_id == kTelemetryFar) {
      const node_id mid = static_cast<node_id>(actor_count / 2);
      if ((src <= mid) != (dst <= mid)) {
        post_recovery_cross_deliveries++;
      }
    }
    if (latency_us >= 0) {
      pair_latency& entry = latencies[{ src, dst }];
      if (entry.count == 0 || latency_us < entry.min_us) {
        entry.min_us = latency_us;
      }
      entry.max_us = std::max(entry.max_us, latency_us);
      entry.total_us += latency_us;
      entry.count++;
    }
  }

  void network_scenario::send_traffic_round(bool count_for_churn) {
    const uint32_t round = static_cast<uint32_t>(rounds_sent);
    const uint8_t hop_budget = static_cast<uint8_t>(actor_count);
    for (size_t i = 1; i <= actor_count; ++i) {
      scenario_actor* actor = static_cast<scenario_actor*>(mesh->actor(static_cast<node_id>(i)));
      if (actor == nullptr) {
        continue;
      }
      /// (near/far are windef.h macro corpses — hence the _id suffixes)
      const node_id near_id = static_cast<node_id>(i % actor_count + 1);
      const node_id far_id = far_target(static_cast<node_id>(i));

      if (actor->send_routed(near_id, kTelemetryNear, telemetry_payload(now, round), hop_budget)) {
        steady_sent++;
      } else {
        scenario_refused++;
      }
      if (far_id != near_id && far_id != static_cast<node_id>(i)) {
        if (actor->send_routed(far_id, kTelemetryFar, telemetry_payload(now, round), hop_budget)) {
          steady_sent++;
        } else {
          /// no route during the partition is expected data, not an error
          scenario_refused++;
        }
      }
    }
    rounds_sent++;
  }

  void network_scenario::apply_cut() {
    unroutable_at_cut_start = relay_drops;
    refused_at_cut_start = scenario_refused;
    for (const planned_link& planned : plan) {
      if (!planned.crosses_cut) {
        continue;
      }
      if (const link_record* record = mesh->link_between(planned.a, planned.b); record != nullptr) {
        mesh->close_link(record->link_id, link_close_reason::SHUTDOWN);
      }
    }
    cut_applied = true;
    rebuild_routes();
    CORE_LOG_INFO("[NET-HARNESS] partition applied at round {} (t={} ms)", rounds_sent, now.count() / 1000);
  }

  void network_scenario::reopen_cut() {
    reopened = true;
    topology_rng rng{ seed ^ 0xBEEFull };
    for (const planned_link& planned : plan) {
      if (!planned.crosses_cut) {
        continue;
      }
      const uint64_t endpoint = 0xF0000000ull | (static_cast<uint64_t>(planned.a) << 16) | planned.b;
      fabric->configure_endpoint(endpoint, false);
      mesh->actor(planned.b)->open_listener(net_address::memory_endpoint(endpoint));
      const natural_t link_id = mesh->actor(planned.a)->open_link(net_address::memory_endpoint(endpoint));
      if (link_id != 0) {
        const int64_t latency = rng.range(latency_min_us, latency_max_us);
        link_profile profile;
        profile.latency = microseconds{ latency };
        profile.jitter = microseconds{ static_cast<int64_t>(latency * jitter_pct) };
        if (const link_record* record = mesh->link(link_id); record != nullptr) {
          fabric->set_profile(record->connection_id, profile, profile);
        }
      }
    }
    CORE_LOG_INFO("[NET-HARNESS] partition healing at round {} (t={} ms)", rounds_sent, now.count() / 1000);
  }

  void network_scenario::enter_phase(phase next) {
    current_phase = next;
    phase_ticks = 0;
  }

  void network_scenario::tick_once() {
    now += microseconds{ tick_us };
    total_ticks++;
    phase_ticks++;
    mesh->tick(now);
    fabric->tick(now);

    switch (current_phase) {
      case phase::ESTABLISH: {
        if (all_planned_links_up()) {
          establish_ticks = phase_ticks;
          rebuild_routes();
          CORE_LOG_INFO("[NET-HARNESS] {} links up after {} ticks", mesh->link_count(), establish_ticks);
          enter_phase(phase::STEADY);
          break;
        }
        if (phase_ticks > max_establish_ticks) {
          failures.push_back(std::format("establish: not all links up within {} ticks", max_establish_ticks));
          enter_phase(phase::TEARDOWN);
        }
        break;
      }

      case phase::STEADY: {
        if (phase_ticks % round_interval_ticks == 0 && rounds_sent < steady_rounds) {
          send_traffic_round(false);
        }
        if (rounds_sent >= steady_rounds && phase_ticks >= steady_rounds * round_interval_ticks + drain_ticks) {
          apply_cut();
          enter_phase(phase::CHURN);
        }
        break;
      }

      case phase::CHURN: {
        if (phase_ticks % round_interval_ticks == 0 && churn_rounds_run < churn_rounds) {
          send_traffic_round(true);
          churn_rounds_run++;
          if (churn_rounds_run == churn_rounds / 2) {
            /// the partition's footprint so far: sends refused (no route from the
            ///  origin) or dropped at relays while the cut was open
            partition_unroutable = relay_drops - unroutable_at_cut_start;
            partition_refused_sends = scenario_refused - refused_at_cut_start;
            reopen_cut();
          }
        }
        if (reopened && !routes_rebuilt_after_heal && all_planned_links_up()) {
          rebuild_routes();  // healed links have fresh ids; the director re-routes
          routes_rebuilt_after_heal = true;
        }
        if (churn_rounds_run >= churn_rounds && phase_ticks >= churn_rounds * round_interval_ticks + drain_ticks) {
          /// open the lossy datagram lane 1 <-> 2, shaped after establishment
          const uint64_t endpoint = 0xDA7A0001ull;
          fabric->configure_endpoint(endpoint, true);
          mesh->actor(2)->open_listener(net_address::memory_endpoint(endpoint));
          datagram_link = mesh->actor(1)->open_link(net_address::memory_endpoint(endpoint));
          enter_phase(phase::DATAGRAM);
        }
        break;
      }

      case phase::DATAGRAM: {
        const link_record* lane = mesh->link(datagram_link);
        if (lane == nullptr) {
          failures.push_back("datagram: lane link vanished");
          enter_phase(phase::TEARDOWN);
          break;
        }
        if (lane->state == link_state::UP && datagram_conn == 0) {
          datagram_conn = lane->connection_id;
          link_profile lossy;
          lossy.loss = static_cast<float>(datagram_loss);
          fabric->set_profile(datagram_conn, lossy, {});
        }
        if (datagram_conn != 0 && datagram_sent < datagram_frames) {
          if (mesh->actor(1)->send_on_link(datagram_link, kDatagramLane, telemetry_payload(now, static_cast<uint32_t>(datagram_sent)))) {
            datagram_sent++;
          }
        }
        if (datagram_sent >= datagram_frames) {
          if (datagram_drain_left == 0) {
            datagram_drain_left = drain_ticks;
          } else if (--datagram_drain_left == 0) {
            enter_phase(phase::TEARDOWN);
          }
        }
        break;
      }

      case phase::TEARDOWN: {
        if (phase_ticks == 1) {
          ostd::vector<natural_t> live;
          for (const link_record& record : mesh->links()) {
            live.push_back(record.link_id);
          }
          for (const natural_t id : live) {
            mesh->close_link(id, link_close_reason::SHUTDOWN);
          }
        }
        if (phase_ticks >= 8) {
          enter_phase(phase::DONE);
        }
        break;
      }

      case phase::DONE:
        break;
    }
  }

  bool network_scenario::update(driver& host) {
    for (size_t i = 0; i < ticks_per_frame && current_phase != phase::DONE; ++i) {
      tick_once();
    }
    return current_phase != phase::DONE;
  }

  bool network_scenario::finalize(driver& host) {
    const mesh_counters& counters = mesh->counters();

    /// verdict assembly: every threshold that fails becomes a reason line
    const double delivery_ratio = steady_sent > 0 ? static_cast<double>(steady_delivered) / static_cast<double>(steady_sent) : 0.0;
    if (delivery_ratio < min_delivery_ratio) {
      failures.push_back(std::format("delivery ratio {:.4f} below {:.4f} ({} of {})",
                                     delivery_ratio, min_delivery_ratio, steady_delivered, steady_sent));
    }

    int64_t worst_avg_us = 0;
    for (const auto& [pair, entry] : latencies) {
      if (entry.count > 0) {
        worst_avg_us = std::max(worst_avg_us, entry.total_us / static_cast<int64_t>(entry.count));
      }
    }
    if (worst_avg_us > max_e2e_latency_us) {
      failures.push_back(std::format("worst pair avg latency {} us exceeds {} us", worst_avg_us, max_e2e_latency_us));
    }

    if (partition_unroutable + partition_refused_sends == 0) {
      failures.push_back("churn: the partition left no footprint (no refused/unroutable traffic — cut ineffective)");
    }
    if (post_recovery_cross_deliveries == 0) {
      failures.push_back("churn: no cross-partition deliveries after healing (recovery failed)");
    }

    const double observed_loss = datagram_sent > 0
      ? 1.0 - static_cast<double>(datagram_delivered) / static_cast<double>(datagram_sent)
      : 1.0;
    if (std::abs(observed_loss - datagram_loss) > datagram_loss_tolerance) {
      failures.push_back(std::format("datagram loss {:.3f} outside {:.3f} +/- {:.3f}",
                                     observed_loss, datagram_loss, datagram_loss_tolerance));
    }

    if (counters.protocol_errors != 0 || counters.malformed_frames != 0 || counters.security_failures != 0) {
      failures.push_back(std::format("dirty counters: protocol_errors={} malformed={} security={}",
                                     counters.protocol_errors, counters.malformed_frames, counters.security_failures));
    }
    if (mesh->link_count() != 0) {
      failures.push_back(std::format("teardown left {} live links", mesh->link_count()));
    }

    const bool pass = failures.empty();
    std::string reason = pass
      ? std::format("delivery {:.4f}, worst avg latency {} us, partition footprint {} drops, recovered {} cross frames, datagram loss {:.3f}",
                    delivery_ratio, worst_avg_us, partition_unroutable + partition_refused_sends,
                    post_recovery_cross_deliveries, observed_loss)
      : failures.front();

    nlohmann::json report;
    report["scenario"] = "network";
    report["pass"] = pass;
    report["reason"] = reason;
    report["failures"] = failures;
    report["config"] = {
      { "actors", actor_count },
      { "seed", seed },
      { "tick-us", tick_us },
      { "cross-link-stride", cross_link_stride },
      { "latency-min-us", latency_min_us },
      { "latency-max-us", latency_max_us },
      { "jitter-pct", jitter_pct },
      { "steady-rounds", steady_rounds },
      { "churn-rounds", churn_rounds },
      { "datagram-frames", datagram_frames },
      { "datagram-loss", datagram_loss },
    };
    report["totals"] = {
      { "virtual-ticks", total_ticks },
      { "virtual-duration-ms", now.count() / 1000 },
      { "establish-ticks", establish_ticks },
      { "steady-sent", steady_sent },
      { "steady-delivered", steady_delivered },
      { "delivery-ratio", delivery_ratio },
      { "partition-unroutable-drops", partition_unroutable },
      { "partition-refused-sends", partition_refused_sends },
      { "post-recovery-cross-deliveries", post_recovery_cross_deliveries },
      { "datagram-sent", datagram_sent },
      { "datagram-delivered", datagram_delivered },
      { "datagram-observed-loss", observed_loss },
      { "relay-drops", relay_drops },
      { "refused-sends", counters.refused_sends },
      { "no-actor-drops", counters.no_actor_drops },
      { "protocol-errors", counters.protocol_errors },
      { "malformed-frames", counters.malformed_frames },
      { "security-failures", counters.security_failures },
    };
    for (const auto& [pair, entry] : latencies) {
      if (entry.count == 0) {
        continue;
      }
      report["pair-latencies"].push_back({
        { "src", pair.first },
        { "dst", pair.second },
        { "count", entry.count },
        { "min-us", entry.min_us },
        { "avg-us", entry.total_us / static_cast<int64_t>(entry.count) },
        { "max-us", entry.max_us },
      });
    }

    filepath out_path{ report_path };
    if (out_path.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out_path.parent_path(), ec);
    }
    std::ofstream out(out_path);
    out << report.dump(2);
    out.close();

    if (pass) {
      CORE_LOG_INFO("[NET-HARNESS] PASS - {}", reason);
    } else {
      for (const std::string& failure : failures) {
        CORE_LOG_ERROR("[NET-HARNESS] FAIL - {}", failure);
      }
    }
    CORE_LOG_INFO("[NET-HARNESS] report written to '{}'", report_path);

    mesh = nullptr;
    fabric = nullptr;
    return pass;
  }

}  // namespace other
