/**
 * \file tests/harness/src/stress_scenario.cpp
 **/
#include "stress_scenario.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/logger.hpp"
#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {
  namespace {

    double percentile(ostd::vector<double> sorted_ms, double p) {
      if (sorted_ms.empty()) {
        return 0.0;
      }
      const size_t index = std::min(sorted_ms.size() - 1, static_cast<size_t>(p * static_cast<double>(sorted_ms.size() - 1)));
      return sorted_ms[index];
    }

  }  // namespace

  void stress_scenario::initialize(driver& host, const config_table& config) {
    scene_name = config.get_value<std::string>("harness.stress.scene", scene_name);
    duration_seconds = config.get_value<double>("harness.stress.duration-seconds", duration_seconds);
    warmup_seconds = config.get_value<double>("harness.stress.warmup-seconds", warmup_seconds);
    sample_interval_seconds = config.get_value<double>("harness.stress.sample-interval-seconds", sample_interval_seconds);
    max_frame_stall_seconds = config.get_value<double>("harness.stress.max-frame-stall-seconds", max_frame_stall_seconds);
    frame_budget_ms = config.get_value<double>("harness.stress.frame-budget-ms", frame_budget_ms);
    max_live_allocation_growth = config.get_value<size_t>("harness.stress.max-live-allocation-growth", max_live_allocation_growth);
    max_used_memory_growth_bytes = config.get_value<size_t>("harness.stress.max-used-memory-growth-bytes", max_used_memory_growth_bytes);
    report_path = config.get_value<std::string>("harness.stress.report-path", report_path);

    CORE_LOG_INFO("[STRESS] scene '{}' | duration {}s | warmup {}s | stall gate {}s | p95 budget {}ms",
                  scene_name, duration_seconds, warmup_seconds, max_frame_stall_seconds, frame_budget_ms);
  }

  double stress_scenario::elapsed_now() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
  }

  void stress_scenario::activate_stress_scene(driver& host) {
    auto& scenes = host.get_kernel().get_core_system<scene_system>();
    target_scene_id = scenes.get_id_of_scene(scene_name);

    scene* active = scenes.get_active_scene();
    if (active != nullptr && active->id == target_scene_id) {
      measuring = true;  /// already there (project starting scene IS the stress scene)
      start_time = std::chrono::steady_clock::now();
      last_frame_time = start_time;
      return;
    }

    CORE_LOG_INFO("[STRESS] activating '{}' ({:#x})", scene_name, target_scene_id);
    scenes.set_scene_to_active(target_scene_id);
  }

  bool stress_scenario::update(driver& host) {
    if (finishing) {
      return false;
    }
    if (!scene_ready) {
      return true;  /// project still loading its starting scene
    }
    if (!switch_requested) {
      switch_requested = true;
      activate_stress_scene(host);
      return true;
    }
    if (!measuring) {
      return true;  /// waiting for the stress scene activation callback
    }

    const auto now = std::chrono::steady_clock::now();
    const double dt_ms = std::chrono::duration<double, std::milli>(now - last_frame_time).count();
    last_frame_time = now;
    frame_index++;

    const double elapsed = elapsed_now();
    if (elapsed >= warmup_seconds) {
      frame_ms.push_back(dt_ms);
    }
    if (elapsed >= next_sample_at) {
      samples.push_back(take_memory_sample(elapsed, frame_index));
      next_sample_at += sample_interval_seconds;
    }
    if (elapsed >= duration_seconds) {
      CORE_LOG_INFO("[STRESS] duration reached after {} frames", frame_index);
      samples.push_back(take_memory_sample(elapsed, frame_index));
      finishing = true;
      return false;
    }
    return true;
  }

  void stress_scenario::scene_activated(driver& host, natural_t scene_id) {
    scene_ready = true;
    if (switch_requested && scene_id == target_scene_id && !measuring) {
      measuring = true;
      start_time = std::chrono::steady_clock::now();
      last_frame_time = start_time;
      next_sample_at = 0.0;
      CORE_LOG_INFO("[STRESS] stress scene active, measuring");
    }
  }

  bool stress_scenario::finalize(driver& host) {
    ostd::vector<double> sorted = frame_ms;
    std::sort(sorted.begin(), sorted.end());

    const size_t frames = sorted.size();
    double total_ms = 0.0;
    for (const double ms : sorted) {
      total_ms += ms;
    }
    const double avg_ms = frames > 0 ? total_ms / static_cast<double>(frames) : 0.0;
    const double p50 = percentile(sorted, 0.50);
    const double p95 = percentile(sorted, 0.95);
    const double p99 = percentile(sorted, 0.99);
    const double max_ms = frames > 0 ? sorted.back() : 0.0;

    int64_t live_growth = 0;
    int64_t used_growth = 0;
    bool memory_ok = true;
    if (samples.size() >= 2) {
      live_growth = static_cast<int64_t>(samples.back().arena_live_allocations) - static_cast<int64_t>(samples.front().arena_live_allocations);
      used_growth = static_cast<int64_t>(samples.back().arena_used_memory) - static_cast<int64_t>(samples.front().arena_used_memory);
      memory_ok = live_growth <= static_cast<int64_t>(max_live_allocation_growth) &&
                  used_growth <= static_cast<int64_t>(max_used_memory_growth_bytes);
    }

    const bool completed = measuring && frames > 0;
    const bool stall_ok = max_ms <= max_frame_stall_seconds * 1000.0;
    const bool budget_ok = frame_budget_ms <= 0.0 || p95 <= frame_budget_ms;
    const bool pass = completed && stall_ok && budget_ok && memory_ok;

    std::string reason;
    if (!completed) {
      reason = "stress scene never activated or produced no frames";
    } else {
      reason = std::format("{} frames | avg {:.2f}ms p95 {:.2f}ms max {:.2f}ms | growth {} live / {} bytes{}{}",
                           frames, avg_ms, p95, max_ms, live_growth, used_growth,
                           stall_ok ? "" : " | FRAME STALL", budget_ok ? "" : " | OVER BUDGET");
    }

    json::json report;
    report["scenario"] = "stress";
    report["pass"] = pass;
    report["reason"] = reason;
    report["scene"] = scene_name;
    report["config"] = {
      { "duration-seconds", duration_seconds },
      { "warmup-seconds", warmup_seconds },
      { "max-frame-stall-seconds", max_frame_stall_seconds },
      { "frame-budget-ms", frame_budget_ms },
      { "max-live-allocation-growth", max_live_allocation_growth },
      { "max-used-memory-growth-bytes", max_used_memory_growth_bytes },
    };
    report["frames"] = frames;
    report["frame-times-ms"] = { { "avg", avg_ms }, { "p50", p50 }, { "p95", p95 }, { "p99", p99 }, { "max", max_ms } };
    report["avg-fps"] = avg_ms > 0.0 ? 1000.0 / avg_ms : 0.0;
    report["live-allocation-growth"] = live_growth;
    report["used-memory-growth-bytes"] = used_growth;
    for (const auto& sample : samples) {
      report["memory-samples"].push_back({
        { "t", sample.elapsed_seconds },
        { "frame", sample.frame_index },
        { "arena-live-allocations", sample.arena_live_allocations },
        { "arena-used-memory", sample.arena_used_memory },
        { "process-working-set", sample.process_working_set },
      });
    }

    filepath out_path{ report_path };
    if (out_path.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out_path.parent_path(), ec);
    }
    std::ofstream out(out_path);
    out << report.dump(2);

    if (pass) {
      CORE_LOG_INFO("[STRESS] PASS - {}", reason);
    } else {
      CORE_LOG_ERROR("[STRESS] FAIL - {}", reason);
    }
    return pass;
  }

}  // namespace other
