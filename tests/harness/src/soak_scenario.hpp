/**
 * \file tests/harness/src/soak_scenario.hpp
 *
 * long-running memory soak: runs a real project (optionally cycling scenes for load/unload
 *  churn), samples arena + process counters on an interval, fails if memory grows after warmup
 **/
#ifndef OTHER_TESTS_HARNESS_SOAK_SCENARIO_HPP
#define OTHER_TESTS_HARNESS_SOAK_SCENARIO_HPP

#include <chrono>
#include <string>

#include "harness_scenario.hpp"
#include "memory_sampler.hpp"

namespace other {

  class soak_scenario : public harness_scenario {
   public:
    std::string_view name() const override { return "soak"; }

    void initialize(driver& host, const config_table& config) override;
    bool update(driver& host) override;
    void scene_activated(driver& host, natural_t scene_id) override;
    bool finalize(driver& host) override;

   private:
    /// config (all under [harness.soak])
    double duration_seconds = 120.0;
    double warmup_seconds = 30.0;
    double sample_interval_seconds = 5.0;
    double scene_cycle_seconds = 0.0;  /// 0 disables cycling
    ostd::vector<std::string> scene_cycle;
    size_t max_live_allocation_growth = 128;
    size_t max_used_memory_growth_bytes = 1024 * 1024;
    std::string report_path = "logs/soak-report.json";

    /// run state
    bool started = false;
    bool finishing = false;
    bool scene_ready = false;
    uint64_t frame_index = 0;
    size_t next_cycle_index = 0;
    natural_t scenes_activated = 0;
    std::chrono::steady_clock::time_point start_time;
    double next_sample_at = 0.0;
    double next_cycle_at = 0.0;

    ostd::vector<memory_sample> samples;

    double elapsed_now() const;
    void take_sample(double elapsed);
    void cycle_scene(driver& host);
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_SOAK_SCENARIO_HPP
