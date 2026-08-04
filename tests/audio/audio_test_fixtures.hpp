/**
 * \file tests/audio/audio_test_fixtures.hpp
 *
 * generated audio fixtures: in-memory clips for environment tests and a tiny
 * RIFF writer for pipeline/preview tests (shared with the future editor suite)
 **/
#ifndef OTHER_TESTS_AUDIO_AUDIO_TEST_FIXTURES_HPP
#define OTHER_TESTS_AUDIO_AUDIO_TEST_FIXTURES_HPP

#include <cmath>
#include <cstdint>
#include <fstream>
#include <numbers>

#include "audio/audio_clip.hpp"

namespace other {

  /// interleaved f32 sine clip; deterministic and engine-free
  inline audio_clip make_test_clip(float seconds, uint32_t sample_rate = 48000, uint32_t channels = 1, float frequency = 440.f) {
    audio_clip clip{};
    clip.channels = channels;
    clip.sample_rate = sample_rate;
    clip.frames = static_cast<uint64_t>(seconds * static_cast<float>(sample_rate));
    clip.seconds = seconds;
    clip.pcm.resize(clip.frames * channels);
    for (uint64_t f = 0; f < clip.frames; ++f) {
      const float sample = 0.25f * std::sin(2.f * std::numbers::pi_v<float> * frequency * (static_cast<float>(f) / static_cast<float>(sample_rate)));
      for (uint32_t c = 0; c < channels; ++c) {
        clip.pcm[f * channels + c] = sample;
      }
    }
    return clip;
  }

  /// minimal 16-bit PCM RIFF/WAVE writer
  inline bool write_test_wav(const filepath& path, float seconds, uint32_t sample_rate = 8000, uint32_t channels = 1, float frequency = 440.f) {
    const uint32_t frames = static_cast<uint32_t>(seconds * static_cast<float>(sample_rate));
    const uint16_t bits = 16;
    const uint16_t block_align = static_cast<uint16_t>(channels * (bits / 8));
    const uint32_t byte_rate = sample_rate * block_align;
    const uint32_t data_size = frames * block_align;

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
      return false;
    }

    const auto put_u32 = [&out](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };
    const auto put_u16 = [&out](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };

    out.write("RIFF", 4);
    put_u32(36 + data_size);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    put_u32(16);
    put_u16(1);  // PCM
    put_u16(static_cast<uint16_t>(channels));
    put_u32(sample_rate);
    put_u32(byte_rate);
    put_u16(block_align);
    put_u16(bits);
    out.write("data", 4);
    put_u32(data_size);

    for (uint32_t f = 0; f < frames; ++f) {
      const float sample = 0.25f * std::sin(2.f * std::numbers::pi_v<float> * frequency * (static_cast<float>(f) / static_cast<float>(sample_rate)));
      const int16_t quantized = static_cast<int16_t>(sample * 32767.f);
      for (uint32_t c = 0; c < channels; ++c) {
        out.write(reinterpret_cast<const char*>(&quantized), 2);
      }
    }
    return out.good();
  }

}  // namespace other

#endif  // OTHER_TESTS_AUDIO_AUDIO_TEST_FIXTURES_HPP
