/**
 * \file tests/harness/src/stress_scenario.hpp
 *
 * frame-load stress on the stress-arena scene: frame times + memory, gated on
 * completion/stall/leak. the same scene is the human tracy target (Profile build).
 **/
#ifndef OTHER_TESTS_HARNESS_STRESS_SCENARIO_HPP
#define OTHER_TESTS_HARNESS_STRESS_SCENARIO_HPP

#include <chrono>
#include <string>

#include "harness_scenario.hpp"
#include "memory_sampler.hpp"

namespace other {

  class stress_scenario : public harness_scenario {
   public:
    std::string_view name() const override { return "stress"; }

    void initialize(driver& host, const config_table& config) override;
    bool update(driver& host) override;
    void scene_activated(driver& host, natural_t scene_id) override;
    bool finalize(driver& host) override;

   private:
    /// config (all under [harness.stress])
    std::string scene_name = "stress-arena";
    double duration_seconds = 90.0;
    double warmup_seconds = 10.0;
    double sample_interval_seconds = 5.0;
    double max_frame_stall_seconds = 5.0;  /// hang detector: any single frame above this fails
    double frame_budget_ms = 0.0;          /// optional p95 gate; 0 = report only
    size_t max_live_allocation_growth = 256;
    size_t max_used_memory_growth_bytes = 4 * 1024 * 1024;
    std::string report_path = "logs/stress-report.json";

    /// run state
    bool scene_ready = false;
    bool switch_requested = false;
    bool measuring = false;
    bool finishing = false;
    uint64_t frame_index = 0;
    natural_t target_scene_id = 0;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_frame_time;
    double next_sample_at = 0.0;

    ostd::vector<double> frame_ms;  /// post-warmup frame times
    ostd::vector<memory_sample> samples;

    double elapsed_now() const;
    void activate_stress_scene(driver& host);
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_STRESS_SCENARIO_HPP
