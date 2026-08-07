/**
 * \file audio/audio_import.cpp
 **/
#include "audio/audio_import.hpp"

#include <filesystem>
#include <format>

#include <miniaudio/miniaudio.h>
#include <toml++/toml.hpp>

#include "core/profiler.hpp"

namespace other {

  namespace {

    constexpr float kMaxSidecarVolume = 2.f;
    constexpr float kStreamAdvisorySeconds = 30.f;

    /// seconds -> frames against the decoded clip, clamped and swap-corrected
    void apply_loop_region(const audio_sidecar& sidecar, audio_clip& clip, ostd::vector<std::string>& warnings) {
      clip.default_loop = sidecar.loop;
      if (sidecar.loop_start_seconds == 0.0 && sidecar.loop_end_seconds == 0.0) {
        return;
      }

      double start_seconds = sidecar.loop_start_seconds;
      double end_seconds = sidecar.loop_end_seconds;
      if (end_seconds != 0.0 && end_seconds < start_seconds) {
        warnings.push_back(std::format("loop region inverted ({}s > {}s); swapped", start_seconds, end_seconds));
        std::swap(start_seconds, end_seconds);
      }

      const auto to_frames = [&clip](double seconds) -> uint64_t {
        if (seconds <= 0.0) {
          return 0;
        }
        const uint64_t frame = static_cast<uint64_t>(seconds * static_cast<double>(clip.sample_rate));
        return std::min<uint64_t>(frame, clip.frames);
      };

      clip.loop.start_frame = to_frames(start_seconds);
      clip.loop.end_frame = to_frames(end_seconds);
      if ((sidecar.loop_start_seconds > 0.0 && clip.loop.start_frame >= clip.frames) ||
          (sidecar.loop_end_seconds > static_cast<double>(clip.seconds))) {
        warnings.push_back("loop region clamped to clip duration");
      }
    }

  }  // namespace

  sidecar_parse_result parse_audio_sidecar(const filepath& audio_absolute) {
    PROFILE_SECTION("parse_audio_sidecar");
    sidecar_parse_result out{};

    filepath sidecar_path = audio_absolute;
    sidecar_path += ".odecl.toml";
    if (!std::filesystem::exists(sidecar_path)) {
      return out;
    }

    toml::table root;
    try {
      root = toml::parse_file(sidecar_path.string());
    } catch (const toml::parse_error& e) {
      out.warnings.push_back(std::format("malformed sidecar '{}' ({}); using defaults", sidecar_path.string(), std::string{ e.description() }));
      return out;
    } catch (const std::exception& e) {
      out.warnings.push_back(std::format("malformed sidecar '{}' ({}); using defaults", sidecar_path.string(), e.what()));
      return out;
    }
    const int64_t schema_version = root["schema-version"].value_or<int64_t>(1);
    if (schema_version > 1) {
      out.warnings.push_back(std::format("sidecar schema-version {} is newer than supported (1); best-effort parse", schema_version));
    }

    const toml::table* audio = root["audio"].as_table();
    if (audio == nullptr) {
      out.warnings.push_back(std::format("sidecar '{}' has no [audio] table; using defaults", sidecar_path.string()));
      return out;
    }

    out.sidecar.loop = (*audio)["loop"].value_or(false);
    out.sidecar.loop_start_seconds = (*audio)["loop-start"].value_or(0.0);
    out.sidecar.loop_end_seconds = (*audio)["loop-end"].value_or(0.0);
    out.sidecar.volume = (*audio)["volume"].value_or(1.0f);
    out.sidecar.stream = (*audio)["stream"].value_or(false);

    if (out.sidecar.volume < 0.f || out.sidecar.volume > kMaxSidecarVolume) {
      out.warnings.push_back(std::format("sidecar volume {} clamped to [0, {}]", out.sidecar.volume, kMaxSidecarVolume));
      out.sidecar.volume = std::clamp(out.sidecar.volume, 0.f, kMaxSidecarVolume);
    }
    return out;
  }

  audio_import_result import_audio(const filepath& absolute) {
    PROFILE_SECTION("import_audio");
    audio_import_result out{};

    sidecar_parse_result sidecar = parse_audio_sidecar(absolute);
    out.warnings = std::move(sidecar.warnings);

    const std::string path = absolute.string();
    ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 0, 0);  /// keep source channels/rate
    ma_decoder decoder;
    if (ma_decoder_init_file(path.c_str(), &decoder_config, &decoder) != MA_SUCCESS) {
      out.error = std::format("failed to open/decode audio file '{}'", path);
      return out;
    }

    ma_uint64 frames = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frames) != MA_SUCCESS || frames == 0) {
      ma_decoder_uninit(&decoder);
      out.error = std::format("audio file '{}' has no decodable frames", path);
      return out;
    }

    audio_clip clip{};
    clip.channels = decoder.outputChannels;
    clip.sample_rate = decoder.outputSampleRate;
    clip.frames = frames;
    clip.seconds = static_cast<float>(frames) / static_cast<float>(clip.sample_rate);
    clip.default_gain = sidecar.sidecar.volume;

    if (sidecar.sidecar.stream) {
      ma_decoder_uninit(&decoder);
      clip.streamed = true;
      clip.source_absolute = absolute;
      apply_loop_region(sidecar.sidecar, clip, out.warnings);
      out.clip = std::move(clip);
      return out;
    }

    if (clip.seconds > kStreamAdvisorySeconds) {
      out.warnings.push_back(std::format("{:.1f}s clip is not flagged stream = true; decoding whole file", clip.seconds));
    }

    clip.pcm.resize(clip.frames * clip.channels);
    ma_uint64 total_read = 0;
    {
      PROFILE_SECTION("import_audio--decode_pcm");
      while (total_read < clip.frames) {
        ma_uint64 read = 0;
        const ma_result read_result = ma_decoder_read_pcm_frames(&decoder, clip.pcm.data() + total_read * clip.channels, clip.frames - total_read, &read);
        total_read += read;
        if (read == 0 || read_result == MA_AT_END) {
          break;
        }
        if (read_result != MA_SUCCESS) {
          break;
        }
      }
    }
    ma_decoder_uninit(&decoder);

    if (total_read == 0) {
      out.error = std::format("audio file '{}' decoded zero frames", path);
      return out;
    }
    if (total_read != clip.frames) {
      /// tolerated: some containers over-report length; shrink to what decoded
      out.warnings.push_back(std::format("decoded {} of {} reported frames; using decoded length", total_read, clip.frames));
      clip.frames = total_read;
      clip.seconds = static_cast<float>(total_read) / static_cast<float>(clip.sample_rate);
      clip.pcm.resize(clip.frames * clip.channels);
    }

    apply_loop_region(sidecar.sidecar, clip, out.warnings);
    out.clip = std::move(clip);
    return out;
  }

}  // namespace other
