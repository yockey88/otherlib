/**
 * \file tests/harness/src/soak_scenario.cpp
 **/
#include "soak_scenario.hpp"

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/logger.hpp"
#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"

namespace other {

  void soak_scenario::initialize(driver& host, const config_table& config) {
    duration_seconds = config.get_value<double>("harness.soak.duration-seconds", duration_seconds);
    warmup_seconds = config.get_value<double>("harness.soak.warmup-seconds", warmup_seconds);
    sample_interval_seconds = config.get_value<double>("harness.soak.sample-interval-seconds", sample_interval_seconds);
    scene_cycle_seconds = config.get_value<double>("harness.soak.scene-cycle-seconds", scene_cycle_seconds);
    scene_cycle = config.get_value<ostd::vector<std::string>>("harness.soak.scene-cycle", {});
    max_live_allocation_growth = config.get_value<size_t>("harness.soak.max-live-allocation-growth", max_live_allocation_growth);
    max_used_memory_growth_bytes = config.get_value<size_t>("harness.soak.max-used-memory-growth-bytes", max_used_memory_growth_bytes);
    report_path = config.get_value<std::string>("harness.soak.report-path", report_path);

    if (warmup_seconds >= duration_seconds) {
      CORE_LOG_WARN("[SOAK] warmup ({}s) >= duration ({}s); the verdict will have no post-warmup baseline", warmup_seconds, duration_seconds);
    }

    CORE_LOG_INFO("[SOAK] duration {}s | warmup {}s | sample every {}s | scene cycle every {}s ({} scenes)",
                  duration_seconds, warmup_seconds, sample_interval_seconds, scene_cycle_seconds, scene_cycle.size());
  }

  double soak_scenario::elapsed_now() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
  }

  bool soak_scenario::update(driver& host) {
    if (finishing) {
      return false;
    }

    if (!started) {
      started = true;
      start_time = std::chrono::steady_clock::now();
      next_sample_at = 0.0;
      next_cycle_at = scene_cycle_seconds;
      CORE_LOG_INFO("[SOAK] run started");
    }

    frame_index++;
    double elapsed = elapsed_now();

    if (elapsed >= next_sample_at) {
      take_sample(elapsed);
      next_sample_at += sample_interval_seconds;
    }

    if (scene_cycle_seconds > 0.0 && !scene_cycle.empty() && scene_ready && elapsed >= next_cycle_at) {
      cycle_scene(host);
      next_cycle_at += scene_cycle_seconds;
    }

    if (elapsed >= duration_seconds) {
      CORE_LOG_INFO("[SOAK] duration reached after {} frames, shutting down", frame_index);
      take_sample(elapsed);
      finishing = true;
      return false;
    }

    return true;
  }

  void soak_scenario::scene_activated(driver& host, natural_t scene_id) {
    scenes_activated++;
    scene_ready = true;
    CORE_LOG_INFO("[SOAK] scene activated: {:#x} (activation #{})", scene_id, scenes_activated);
  }

  void soak_scenario::take_sample(double elapsed) {
    memory_sample sample = take_memory_sample(elapsed, frame_index);
    samples.push_back(sample);
    CORE_LOG_INFO("[SOAK] t={:.1f}s frame={} | arena live={} used={} KiB | process ws={} MiB",
                  sample.elapsed_seconds, sample.frame_index, sample.arena_live_allocations,
                  sample.arena_used_memory / 1024, sample.process_working_set / (1024 * 1024));
  }

  void soak_scenario::cycle_scene(driver& host) {
    const std::string& next_name = scene_cycle[next_cycle_index % scene_cycle.size()];
    next_cycle_index++;

    auto& scenes = host.get_kernel().get_core_system<scene_system>();
    natural_t next_id = scenes.get_id_of_scene(next_name);
    scene* active = scenes.get_active_scene();
    if (active != nullptr && active->id == next_id) {
      return;
    }

    CORE_LOG_INFO("[SOAK] cycling scene -> '{}' ({:#x})", next_name, next_id);
    scenes.set_scene_to_active(next_id);
  }

  bool soak_scenario::finalize(driver& host) {
    /// one last sample after the driver tore the project/scenes down: shows how much
    ///  memory the shutdown path actually recovered
    memory_sample post_shutdown = take_memory_sample(elapsed_now(), frame_index);

    const memory_sample* baseline = nullptr;
    for (const auto& sample : samples) {
      if (sample.elapsed_seconds >= warmup_seconds) {
        baseline = &sample;
        break;
      }
    }

    bool pass = false;
    std::string verdict_reason;
    int64_t live_growth = 0;
    int64_t used_growth = 0;

    if (samples.size() < 2 || baseline == nullptr || baseline == &samples.back()) {
      verdict_reason = "insufficient samples after warmup; run longer or lower warmup-seconds";
    } else {
      const memory_sample& final_sample = samples.back();
      live_growth = static_cast<int64_t>(final_sample.arena_live_allocations) - static_cast<int64_t>(baseline->arena_live_allocations);
      used_growth = static_cast<int64_t>(final_sample.arena_used_memory) - static_cast<int64_t>(baseline->arena_used_memory);

      bool live_ok = live_growth <= static_cast<int64_t>(max_live_allocation_growth);
      bool used_ok = used_growth <= static_cast<int64_t>(max_used_memory_growth_bytes);
      pass = live_ok && used_ok;
      verdict_reason = std::format("post-warmup growth: {} live allocations (max {}), {} bytes used (max {})",
                                   live_growth, max_live_allocation_growth, used_growth, max_used_memory_growth_bytes);
    }

    json::json report;
    report["scenario"] = "soak";
    report["pass"] = pass;
    report["reason"] = verdict_reason;
    report["config"] = {
      { "duration-seconds", duration_seconds },
      { "warmup-seconds", warmup_seconds },
      { "sample-interval-seconds", sample_interval_seconds },
      { "scene-cycle-seconds", scene_cycle_seconds },
      { "max-live-allocation-growth", max_live_allocation_growth },
      { "max-used-memory-growth-bytes", max_used_memory_growth_bytes },
    };
    report["frames"] = frame_index;
    report["scene-activations"] = scenes_activated;
    report["live-allocation-growth"] = live_growth;
    report["used-memory-growth-bytes"] = used_growth;

    auto sample_to_json = [](const memory_sample& sample) {
      return json::json{
        { "t", sample.elapsed_seconds },
        { "frame", sample.frame_index },
        { "arena-total-allocations", sample.arena_total_allocations },
        { "arena-live-allocations", sample.arena_live_allocations },
        { "arena-requested-memory", sample.arena_requested_memory },
        { "arena-used-memory", sample.arena_used_memory },
        { "process-working-set", sample.process_working_set },
        { "process-private-bytes", sample.process_private_bytes },
      };
    };
    for (const auto& sample : samples) {
      report["samples"].push_back(sample_to_json(sample));
    }
    report["post-shutdown-sample"] = sample_to_json(post_shutdown);

    filepath out_path{ report_path };
    if (out_path.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out_path.parent_path(), ec);
    }
    std::ofstream out(out_path);
    out << report.dump(2);
    out.close();

    if (pass) {
      CORE_LOG_INFO("[SOAK] PASS - {}", verdict_reason);
    } else {
      CORE_LOG_ERROR("[SOAK] FAIL - {}", verdict_reason);
    }
    CORE_LOG_INFO("[SOAK] report written to '{}'", report_path);
    return pass;
  }

}  // namespace other
