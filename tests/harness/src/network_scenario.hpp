/**
 * \file tests/harness/src/network_scenario.hpp
 *
 * deterministic peer-mesh scenario: N actors on one mesh run establish -> steady -> churn ->
 *  datagram -> teardown on a virtual clock; verdict + metrics land in logs/network-report.json
 **/
#ifndef OTHER_TESTS_HARNESS_NETWORK_SCENARIO_HPP
#define OTHER_TESTS_HARNESS_NETWORK_SCENARIO_HPP

#include <string>

#include "core/scope.hpp"
#include "core/time.hpp"

#include "network/memory/memory_transport_provider.hpp"
#include "peer_mesh/peer_mesh.hpp"

#include "harness_scenario.hpp"

namespace other {

  class scenario_actor;

  class network_scenario : public harness_scenario {
   public:
    std::string_view name() const override { return "network"; }

    void initialize(driver& host, const config_table& config) override;
    bool update(driver& host) override;
    bool finalize(driver& host) override;

   private:
    friend class scenario_actor;

    enum class phase : uint8_t { ESTABLISH, STEADY, CHURN, DATAGRAM, TEARDOWN, DONE };

    struct planned_link {
      node_id a = 0;
      node_id b = 0;
      bool crosses_cut = false;
    };

    struct pair_latency {
      natural_t count = 0;
      int64_t min_us = 0;
      int64_t max_us = 0;
      int64_t total_us = 0;
    };

    /// config (all under [harness.network])
    size_t actor_count = 12;
    uint64_t seed = 1;
    int64_t tick_us = 1000;
    size_t ticks_per_frame = 2000;
    size_t cross_link_stride = 4;
    int64_t latency_min_us = 5'000;
    int64_t latency_max_us = 40'000;
    double jitter_pct = 0.2;
    size_t steady_rounds = 150;
    size_t round_interval_ticks = 20;
    size_t drain_ticks = 300;
    size_t churn_rounds = 120;
    size_t datagram_frames = 400;
    double datagram_loss = 0.2;
    double datagram_loss_tolerance = 0.08;
    size_t max_establish_ticks = 400;
    double min_delivery_ratio = 1.0;
    int64_t max_e2e_latency_us = 400'000;
    std::string report_path = "logs/network-report.json";

    /// the network under test
    scope<memory_transport_provider> fabric;
    scope<peer_mesh> mesh;

    /// run state
    phase current_phase = phase::ESTABLISH;
    microseconds now{ 0 };
    natural_t total_ticks = 0;
    natural_t phase_ticks = 0;
    ostd::vector<planned_link> plan;
    natural_t establish_ticks = 0;

    size_t rounds_sent = 0;
    natural_t steady_sent = 0;
    natural_t steady_delivered = 0;
    ostd::map<std::pair<node_id, node_id>, pair_latency> latencies;

    bool cut_applied = false;
    bool reopened = false;
    bool routes_rebuilt_after_heal = false;
    natural_t churn_rounds_run = 0;
    natural_t partition_unroutable = 0;
    natural_t partition_refused_sends = 0;
    natural_t post_recovery_cross_deliveries = 0;
    natural_t unroutable_at_cut_start = 0;
    natural_t refused_at_cut_start = 0;
    /// scenario-protocol counters: relays that found no path, origin sends refused
    natural_t relay_drops = 0;
    natural_t scenario_refused = 0;

    natural_t datagram_link = 0;
    natural_t datagram_conn = 0;
    natural_t datagram_sent = 0;
    natural_t datagram_delivered = 0;
    size_t datagram_drain_left = 0;

    ostd::vector<std::string> failures;

    void tick_once();
    void enter_phase(phase next);
    void open_planned_links();
    bool all_planned_links_up() const;
    void rebuild_routes();
    void send_traffic_round(bool count_for_churn);
    void apply_cut();
    void reopen_cut();
    void record_delivery(node_id src, node_id dst, int64_t latency_us, uint16_t net_id);
    node_id far_target(node_id from) const;
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_NETWORK_SCENARIO_HPP
