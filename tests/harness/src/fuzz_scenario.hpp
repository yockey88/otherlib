/**
 * \file tests/harness/src/fuzz_scenario.hpp
 *
 * seeded fuzz over the untrusted-input parsing seams (frames, scene docs, message
 * codec, config toml); malformed bytes must reject gracefully — no crash, no throw.
 **/
#ifndef OTHER_TESTS_HARNESS_FUZZ_SCENARIO_HPP
#define OTHER_TESTS_HARNESS_FUZZ_SCENARIO_HPP

#include <chrono>
#include <random>
#include <string>

#include "harness_scenario.hpp"

namespace other {

  class fuzz_scenario : public harness_scenario {
   public:
    std::string_view name() const override { return "fuzz"; }

    void initialize(driver& host, const config_table& config) override;
    bool update(driver& host) override;
    bool finalize(driver& host) override;

   private:
    struct target_result {
      std::string name;
      size_t iterations = 0;
      size_t clean_parses = 0;
      size_t graceful_rejects = 0;
      ostd::vector<std::string> violations;
    };

    /// config (all under [harness.fuzz])
    uint64_t seed = 20260808;
    size_t frame_iterations = 20000;
    size_t scene_binary_iterations = 4000;
    size_t scene_toml_iterations = 1500;
    size_t message_iterations = 20000;
    size_t config_iterations = 1500;
    std::string report_path = "logs/fuzz-report.json";

    std::mt19937_64 rng;
    size_t next_target = 0;
    std::chrono::steady_clock::time_point start_time;
    ostd::vector<target_result> results;

    /// mutation helpers; violations are capped per target to keep reports readable
    ostd::vector<uint8_t> random_bytes(size_t max_len);
    ostd::vector<uint8_t> mutate(const ostd::vector<uint8_t>& base);
    void record_violation(target_result& result, std::string_view what);

    target_result fuzz_frames();
    target_result fuzz_scene_binary();
    target_result fuzz_scene_toml();
    target_result fuzz_messages();
    target_result fuzz_config_toml();
  };

}  // namespace other

#endif  // OTHER_TESTS_HARNESS_FUZZ_SCENARIO_HPP
