/**
 * \file tests/harness/src/fuzz_scenario.cpp
 **/
#include "fuzz_scenario.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>

#include <nlohmann/json.hpp>

#include "core/config_table.hpp"
#include "core/logger.hpp"
#include "driver/driver.hpp"

#include "message/message_serialization.hpp"
#include "network/frame.hpp"
#include "network/session/net_messages.hpp"
#include "serialization/scene_serializer.hpp"

namespace other {
  namespace {

    constexpr size_t kMaxRecordedViolations = 8;

    /// template document parsed once per run; component variety keeps the codec paths hot
    constexpr std::string_view kTemplateScene = R"(# fuzz template scene
[scene]
schema-version = 1
name = "fuzz-template"
clear-color = [ 0.1, 0.2, 0.3, 1.0 ]

[[objects]]
id = 1
name = "Root"
tags = [ "fuzz", "root" ]
[objects.components.transform]
local_position = [ 1.0, -2.0, 3.5 ]
local_scale = [ 1.0, 2.0, 1.0 ]
local_rotation_quat = [ 0.0, 0.0, 0.0, 1.0 ]
[objects.components.render]
model_asset_id = "fuzz://model"
material_asset_id = "fuzz://material"
visible = true
tint = [ 1.0, 0.5, 0.25, 1.0 ]
[objects.components.physics]
[objects.components.physics.settings]
body_type = 2
[objects.components.physics.settings.shape]
shape_kind = 2
radius = 0.75

[[objects]]
id = 2
parent = 1
name = "Child"
[objects.components.transform]
local_position = [ 0.0, 1.0, 0.0 ]
local_scale = [ 1.0, 1.0, 1.0 ]
local_rotation_quat = [ 0.0, 0.0, 0.0, 1.0 ]
[objects.components.script]
behaviors = [ "FuzzBehavior" ]

[[objects]]
id = 3
name = "Light"
[objects.components.transform]
local_position = [ 2.0, 4.0, 2.0 ]
local_scale = [ 0.1, 0.1, 0.1 ]
local_rotation_quat = [ 0.0, 0.0, 0.0, 1.0 ]
[objects.components.point-light]
[objects.components.point-light.light]
position = [ 2.0, 4.0, 2.0 ]
color = [ 1.0, 0.44, 0.77, 1.0 ]
)";

    constexpr std::string_view kTemplateConfig = R"([application]
name = "fuzz"
metadata = [ { key = "version", value = "0.1.0" } ]
core-log-level = "info"
[harness]
scenario = "fuzz"
values = [ 1, 2, 3 ]
nested = { a = 1.5, b = "text", c = false }
)";

  }  // namespace

  void fuzz_scenario::initialize(driver& host, const config_table& config) {
    seed = config.get_value<size_t>("harness.fuzz.seed", seed);
    frame_iterations = config.get_value<size_t>("harness.fuzz.frame-iterations", frame_iterations);
    scene_binary_iterations = config.get_value<size_t>("harness.fuzz.scene-binary-iterations", scene_binary_iterations);
    scene_toml_iterations = config.get_value<size_t>("harness.fuzz.scene-toml-iterations", scene_toml_iterations);
    message_iterations = config.get_value<size_t>("harness.fuzz.message-iterations", message_iterations);
    config_iterations = config.get_value<size_t>("harness.fuzz.config-iterations", config_iterations);
    report_path = config.get_value<std::string>("harness.fuzz.report-path", report_path);

    rng.seed(seed);
    start_time = std::chrono::steady_clock::now();
    CORE_LOG_INFO("[FUZZ] seed {} | frames {} | scene-binary {} | scene-toml {} | messages {} | config {}",
                  seed, frame_iterations, scene_binary_iterations, scene_toml_iterations, message_iterations, config_iterations);
  }

  ostd::vector<uint8_t> fuzz_scenario::random_bytes(size_t max_len) {
    std::uniform_int_distribution<size_t> len_dist(0, max_len);
    std::uniform_int_distribution<uint32_t> byte_dist(0, 255);
    ostd::vector<uint8_t> bytes(len_dist(rng));
    for (auto& b : bytes) {
      b = static_cast<uint8_t>(byte_dist(rng));
    }
    return bytes;
  }

  ostd::vector<uint8_t> fuzz_scenario::mutate(const ostd::vector<uint8_t>& base) {
    ostd::vector<uint8_t> bytes = base;
    std::uniform_int_distribution<uint32_t> op_count_dist(1, 4);
    std::uniform_int_distribution<uint32_t> op_dist(0, 5);
    std::uniform_int_distribution<uint32_t> byte_dist(0, 255);

    const uint32_t ops = op_count_dist(rng);
    for (uint32_t i = 0; i < ops && !bytes.empty(); ++i) {
      std::uniform_int_distribution<size_t> pos_dist(0, bytes.size() - 1);
      const size_t pos = pos_dist(rng);
      switch (op_dist(rng)) {
        case 0:  /// bit flip
          bytes[pos] ^= static_cast<uint8_t>(1u << (byte_dist(rng) % 8));
          break;
        case 1:  /// byte overwrite
          bytes[pos] = static_cast<uint8_t>(byte_dist(rng));
          break;
        case 2:  /// truncate
          bytes.resize(pos);
          break;
        case 3: {  /// extend with garbage
          ostd::vector<uint8_t> extra = random_bytes(64);
          bytes.insert(bytes.end(), extra.begin(), extra.end());
        } break;
        case 4: {  /// splice: duplicate a random range in place (copied out first — self-insert is UB)
          const size_t len = std::min<size_t>(bytes.size() - pos, byte_dist(rng) % 32 + 1);
          const ostd::vector<uint8_t> range(bytes.begin() + static_cast<ptrdiff_t>(pos), bytes.begin() + static_cast<ptrdiff_t>(pos + len));
          bytes.insert(bytes.begin() + static_cast<ptrdiff_t>(pos), range.begin(), range.end());
        } break;
        case 5:  /// stomp a 32-bit field with an extreme value (length/count abuse)
          if (bytes.size() >= pos + 4) {
            const uint32_t extreme = (byte_dist(rng) % 2 == 0) ? 0xFFFFFFF0u : 0x7FFFFFFFu;
            std::memcpy(bytes.data() + pos, &extreme, sizeof(extreme));
          }
          break;
      }
    }
    return bytes;
  }

  void fuzz_scenario::record_violation(target_result& result, std::string_view what) {
    if (result.violations.size() < kMaxRecordedViolations) {
      result.violations.push_back(std::format("iteration {}: {}", result.iterations, what));
    }
  }

  fuzz_scenario::target_result fuzz_scenario::fuzz_frames() {
    target_result result{ .name = "frames" };
    std::uniform_int_distribution<uint32_t> id_dist(0, 0xFFFF);
    std::uniform_int_distribution<uint32_t> chunk_dist(1, 97);
    std::uniform_int_distribution<uint32_t> mode_dist(0, 3);

    /// sanity: an unmutated frame must parse clean or the target is meaningless
    {
      const ostd::vector<uint8_t> payload = random_bytes(256);
      const ostd::vector<uint8_t> valid = write_frame(7, payload);
      const frame_parse_result parsed = parse_frame_exact(valid);
      if (parsed.frame.has_value() && parsed.err == frame_parse_result::error::NONE) {
        result.clean_parses++;
      } else {
        record_violation(result, "valid frame failed to parse");
      }
    }

    for (result.iterations = 0; result.iterations < frame_iterations; ++result.iterations) {
      try {
        const uint16_t net_id = static_cast<uint16_t>(id_dist(rng));
        switch (mode_dist(rng)) {
          case 0: {  /// mutated plain frame -> exact parse
            const ostd::vector<uint8_t> bytes = mutate(write_frame(net_id, random_bytes(512)));
            const frame_parse_result parsed = parse_frame_exact(bytes);
            parsed.frame.has_value() ? result.clean_parses++ : result.graceful_rejects++;
          } break;
          case 1: {  /// mutated routed frame; surviving ROUTED payloads run the route header too
            const route_header route{ .src = rng(), .dst = rng(), .ttl = static_cast<uint8_t>(rng() % 16) };
            const ostd::vector<uint8_t> bytes = mutate(write_routed_frame(route, net_id, random_bytes(256)));
            const frame_parse_result parsed = parse_frame_exact(bytes);
            if (parsed.frame.has_value()) {
              result.clean_parses++;
              if ((parsed.frame->flags & static_cast<uint16_t>(frame_flags::ROUTED)) != 0) {
                (void)read_route_header(parsed.frame->payload);
              }
            } else {
              result.graceful_rejects++;
            }
          } break;
          case 2: {  /// stream reassembly: several frames, sometimes mutated, dribbled in chunks
            ostd::vector<uint8_t> stream;
            const size_t frame_count = 1 + rng() % 4;
            for (size_t f = 0; f < frame_count; ++f) {
              ostd::vector<uint8_t> one = write_frame(static_cast<uint16_t>(id_dist(rng)), random_bytes(256));
              if (rng() % 3 == 0) {
                one = mutate(one);
              }
              stream.insert(stream.end(), one.begin(), one.end());
            }
            frame_reader reader;
            size_t offset = 0;
            while (offset < stream.size()) {
              const size_t chunk = std::min<size_t>(chunk_dist(rng), stream.size() - offset);
              reader.feed(std::span(stream.data() + offset, chunk));
              offset += chunk;
              for (frame_parse_result next = reader.next();; next = reader.next()) {
                if (next.frame.has_value()) {
                  result.clean_parses++;
                  continue;
                }
                if (next.err != frame_parse_result::error::NEED_MORE) {
                  result.graceful_rejects++;
                }
                break;
              }
            }
          } break;
          case 3:  /// raw garbage against the route header reader
            (void)read_route_header(random_bytes(64));
            result.graceful_rejects++;
            break;
        }
      } catch (const std::exception& e) {
        record_violation(result, std::format("unexpected exception: {}", e.what()));
      } catch (...) {
        record_violation(result, "unexpected non-std exception");
      }
    }
    return result;
  }

  fuzz_scenario::target_result fuzz_scenario::fuzz_scene_binary() {
    target_result result{ .name = "scene-binary" };
    const serialization::scene_parse_result template_doc = serialization::parse_scene_toml(kTemplateScene);
    if (!template_doc.success()) {
      record_violation(result, std::format("template scene failed to parse: {}", template_doc.error));
      return result;
    }
    const ostd::vector<uint8_t> base = serialization::write_scene_binary(*template_doc.document);

    if (serialization::parse_scene_binary(base).success()) {
      result.clean_parses++;
    } else {
      record_violation(result, "valid scene binary failed to parse");
    }

    for (result.iterations = 0; result.iterations < scene_binary_iterations; ++result.iterations) {
      try {
        const ostd::vector<uint8_t> bytes = (rng() % 8 == 0) ? random_bytes(512) : mutate(base);
        serialization::parse_scene_binary(bytes).success() ? result.clean_parses++ : result.graceful_rejects++;
      } catch (const std::exception& e) {
        record_violation(result, std::format("unexpected exception: {}", e.what()));
      } catch (...) {
        record_violation(result, "unexpected non-std exception");
      }
    }
    return result;
  }

  fuzz_scenario::target_result fuzz_scenario::fuzz_scene_toml() {
    target_result result{ .name = "scene-toml" };
    const serialization::scene_parse_result template_doc = serialization::parse_scene_toml(kTemplateScene);
    if (!template_doc.success()) {
      record_violation(result, std::format("template scene failed to parse: {}", template_doc.error));
      return result;
    }
    const std::string base_text = serialization::write_scene_toml(*template_doc.document);
    const ostd::vector<uint8_t> base(base_text.begin(), base_text.end());

    if (serialization::parse_scene_toml(base_text).success()) {
      result.clean_parses++;
    } else {
      record_violation(result, "valid scene toml failed to parse");
    }

    for (result.iterations = 0; result.iterations < scene_toml_iterations; ++result.iterations) {
      try {
        const ostd::vector<uint8_t> bytes = (rng() % 8 == 0) ? random_bytes(512) : mutate(base);
        const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        serialization::parse_scene_toml(text).success() ? result.clean_parses++ : result.graceful_rejects++;
      } catch (const std::exception& e) {
        record_violation(result, std::format("unexpected exception: {}", e.what()));
      } catch (...) {
        record_violation(result, "unexpected non-std exception");
      }
    }
    return result;
  }

  fuzz_scenario::target_result fuzz_scenario::fuzz_messages() {
    target_result result{ .name = "messages" };
    std::uniform_int_distribution<uint32_t> kind_dist(0, 4);

    auto build_valid = [&](uint32_t kind) -> ostd::vector<uint8_t> {
      switch (kind) {
        case 0: {
          net_welcome msg{ .peer_id = static_cast<uint16_t>(rng()), .host_tick = rng(),
                           .roster_count = static_cast<uint16_t>(rng() % 8), .roster = random_bytes(128) };
          return serialize_direct(msg);
        }
        case 1: {
          net_spawn msg{ .net_id = rng(), .parent_net_id = rng(), .owner_peer = static_cast<uint16_t>(rng()),
                         .name = random_bytes(48), .component_count = static_cast<uint16_t>(rng() % 6), .components = random_bytes(256) };
          return serialize_direct(msg);
        }
        case 2: {
          net_join_snapshot msg{ .host_tick = rng(), .scene_bytes = random_bytes(512) };
          const size_t entries = rng() % 32;
          for (size_t i = 0; i < entries; ++i) {
            msg.table.push_back({ .net_id = rng(), .file_id = rng(), .owner_peer = static_cast<uint16_t>(rng()) });
          }
          return serialize_direct(msg);
        }
        case 3: {
          net_transform_batch msg{ .host_tick = rng() };
          const size_t entries = rng() % 64;
          for (size_t i = 0; i < entries; ++i) {
            msg.entries.push_back({ .net_id = rng() });
          }
          return serialize_direct(msg);
        }
        default: {
          net_game_event msg{ .sender_peer = static_cast<uint16_t>(rng()), .name = random_bytes(32), .payload = random_bytes(200) };
          return serialize_direct(msg);
        }
      }
    };

    /// decode kind is drawn independently of the encoded kind: cross-type decodes must
    ///  still land in value-or-buffer_parsing_error
    auto decode = [&](uint32_t kind, std::span<const uint8_t> bytes) {
      switch (kind) {
        case 0: (void)deserialize_direct<net_welcome>(bytes); break;
        case 1: (void)deserialize_direct<net_spawn>(bytes); break;
        case 2: (void)deserialize_direct<net_join_snapshot>(bytes); break;
        case 3: (void)deserialize_direct<net_transform_batch>(bytes); break;
        default: (void)deserialize_direct<net_game_event>(bytes); break;
      }
    };

    for (uint32_t kind = 0; kind <= 4; ++kind) {
      try {
        decode(kind, build_valid(kind));
        result.clean_parses++;
      } catch (const std::exception& e) {
        record_violation(result, std::format("valid message kind {} failed to decode: {}", kind, e.what()));
      }
    }

    for (result.iterations = 0; result.iterations < message_iterations; ++result.iterations) {
      const uint32_t encode_kind = kind_dist(rng);
      const uint32_t decode_kind = (rng() % 4 == 0) ? kind_dist(rng) : encode_kind;
      try {
        const ostd::vector<uint8_t> bytes = (rng() % 8 == 0) ? random_bytes(256) : mutate(build_valid(encode_kind));
        decode(decode_kind, bytes);
        result.clean_parses++;
      } catch (const buffer_parsing_error&) {
        result.graceful_rejects++;
      } catch (const std::exception& e) {
        record_violation(result, std::format("unexpected exception: {}", e.what()));
      } catch (...) {
        record_violation(result, "unexpected non-std exception");
      }
    }
    return result;
  }

  fuzz_scenario::target_result fuzz_scenario::fuzz_config_toml() {
    target_result result{ .name = "config-toml" };
    const ostd::vector<uint8_t> base(kTemplateConfig.begin(), kTemplateConfig.end());

    if (config_table::load_from_source(kTemplateConfig).valid) {
      result.clean_parses++;
    } else {
      record_violation(result, "valid config failed to parse");
    }

    for (result.iterations = 0; result.iterations < config_iterations; ++result.iterations) {
      try {
        const ostd::vector<uint8_t> bytes = (rng() % 8 == 0) ? random_bytes(256) : mutate(base);
        const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        config_table::load_from_source(text).valid ? result.clean_parses++ : result.graceful_rejects++;
      } catch (const std::exception& e) {
        record_violation(result, std::format("unexpected exception: {}", e.what()));
      } catch (...) {
        record_violation(result, "unexpected non-std exception");
      }
    }
    return result;
  }

  bool fuzz_scenario::update(driver& host) {
    /// one target per driver frame keeps the engine loop honest between batches
    switch (next_target++) {
      case 0: results.push_back(fuzz_frames()); return true;
      case 1: results.push_back(fuzz_scene_binary()); return true;
      case 2: results.push_back(fuzz_scene_toml()); return true;
      case 3: results.push_back(fuzz_messages()); return true;
      case 4: results.push_back(fuzz_config_toml()); return true;
      default: return false;
    }
  }

  bool fuzz_scenario::finalize(driver& host) {
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();

    size_t total_violations = 0;
    for (const auto& result : results) {
      total_violations += result.violations.size();
    }
    const bool all_ran = results.size() == 5;
    const bool pass = all_ran && total_violations == 0;
    const std::string reason = pass
                                 ? std::format("{} targets, no contract violations", results.size())
                                 : std::format("{} targets ran (expected 5), {} contract violations", results.size(), total_violations);

    json::json report;
    report["scenario"] = "fuzz";
    report["pass"] = pass;
    report["reason"] = reason;
    report["seed"] = seed;
    report["elapsed-seconds"] = elapsed;
    for (const auto& result : results) {
      json::json entry;
      entry["name"] = result.name;
      entry["iterations"] = result.iterations;
      entry["clean-parses"] = result.clean_parses;
      entry["graceful-rejects"] = result.graceful_rejects;
      for (const auto& violation : result.violations) {
        entry["violations"].push_back(violation);
      }
      report["targets"].push_back(entry);
      CORE_LOG_INFO("[FUZZ] {}: {} iterations, {} clean, {} rejected, {} violations",
                    result.name, result.iterations, result.clean_parses, result.graceful_rejects, result.violations.size());
    }

    filepath out_path{ report_path };
    if (out_path.has_parent_path()) {
      std::error_code ec;
      std::filesystem::create_directories(out_path.parent_path(), ec);
    }
    std::ofstream out(out_path);
    out << report.dump(2);

    CORE_LOG_INFO("[FUZZ] {} - {} (report '{}')", pass ? "PASS" : "FAIL", reason, report_path);
    return pass;
  }

}  // namespace other
