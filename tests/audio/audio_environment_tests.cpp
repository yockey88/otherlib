/**
 * \file tests/audio/audio_environment_tests.cpp
 *
 * headless audio engine tests — every case runs in forced pump mode (noDevice
 * engine, manually advanced), so results are deterministic and identical on
 * machines with no audio hardware
 **/
#include <filesystem>

#include <gtest/gtest.h>

#include "audio/audio_environment.hpp"

#include "audio/audio_test_fixtures.hpp"
#include "other_test.hpp"

namespace other {

  namespace {

    constexpr natural_t kClipHash = 0xA0D10;
    constexpr natural_t kOtherClipHash = 0xA0D11;

    audio_config pump_config() {
      audio_config config{};
      config.sample_rate = 48000;
      config.force_pump_mode = true;
      return config;
    }

    voice_params looping_params(natural_t clip_hash) {
      voice_params params{};
      params.clip_hash = clip_hash;
      params.looping = true;
      params.spatial = false;
      return params;
    }

  }  // namespace

  class audio_environment_tests : public other_test {};

  TEST_F(audio_environment_tests, initialize_pump_mode_and_reinitialize) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    EXPECT_TRUE(env.is_initialized());
    EXPECT_TRUE(env.pump_mode());
    env.shutdown();
    EXPECT_FALSE(env.is_initialized());

    ASSERT_TRUE(env.initialize(pump_config()));
    EXPECT_TRUE(env.pump_mode());
    env.shutdown();
  }

  TEST_F(audio_environment_tests, zero_sample_rate_falls_back_to_pump_mode) {
    audio_environment env;
    audio_config config{};
    config.sample_rate = 0;
    config.force_pump_mode = false;
    ASSERT_TRUE(env.initialize(config));
    EXPECT_TRUE(env.pump_mode());
    env.shutdown();
  }

  TEST_F(audio_environment_tests, pump_advances_voices_deterministically) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.1f));

    voice_params params{};
    params.clip_hash = kClipHash;
    params.spatial = false;
    const voice_id voice = env.start_voice(params);
    ASSERT_NE(voice, 0u);
    EXPECT_EQ(env.live_voice_count(), 1u);

    env.pump(0.05);
    EXPECT_FALSE(env.voice_finished(voice));
    env.pump(0.06);
    EXPECT_TRUE(env.voice_finished(voice));

    env.stop_voice(voice);
    EXPECT_EQ(env.live_voice_count(), 0u);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, looping_voice_never_finishes) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));

    const voice_id voice = env.start_voice(looping_params(kClipHash));
    ASSERT_NE(voice, 0u);
    env.pump(0.5);
    EXPECT_FALSE(env.voice_finished(voice));

    env.stop_voice(voice);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, voice_pool_exhaustion_refuses_then_recovers) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));

    ostd::vector<voice_id> voices;
    for (size_t i = 0; i < kMaxVoices; ++i) {
      const voice_id voice = env.start_voice(looping_params(kClipHash));
      ASSERT_NE(voice, 0u) << "voice " << i << " failed to start";
      voices.push_back(voice);
    }
    EXPECT_EQ(env.live_voice_count(), kMaxVoices);
    EXPECT_EQ(env.start_voice(looping_params(kClipHash)), 0u);

    env.stop_voice(voices.back());
    voices.pop_back();
    EXPECT_NE(env.start_voice(looping_params(kClipHash)), 0u);

    env.stop_all_voices();
    EXPECT_EQ(env.live_voice_count(), 0u);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, bus_volumes_roundtrip) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));

    env.set_bus_volume(audio_bus::MASTER, 0.5f);
    env.set_bus_volume(audio_bus::MUSIC, 0.25f);
    env.set_bus_volume(audio_bus::SFX, 0.75f);
    env.set_bus_volume(audio_bus::UI, 0.1f);

    EXPECT_FLOAT_EQ(env.bus_volume(audio_bus::MASTER), 0.5f);
    EXPECT_FLOAT_EQ(env.bus_volume(audio_bus::MUSIC), 0.25f);
    EXPECT_FLOAT_EQ(env.bus_volume(audio_bus::SFX), 0.75f);
    EXPECT_FLOAT_EQ(env.bus_volume(audio_bus::UI), 0.1f);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, clip_registry_roundtrip_and_revisions) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));

    EXPECT_EQ(env.get_clip(kClipHash), nullptr);
    EXPECT_EQ(env.clip_revision(kClipHash), 0u);

    env.add_clip(kClipHash, make_test_clip(0.1f, 48000, 2));
    const audio_clip* clip = env.get_clip(kClipHash);
    ASSERT_NE(clip, nullptr);
    EXPECT_EQ(clip->channels, 2u);
    EXPECT_EQ(clip->frames, 4800u);
    EXPECT_EQ(env.clip_revision(kClipHash), 1u);

    env.remove_clip(kClipHash);
    EXPECT_EQ(env.get_clip(kClipHash), nullptr);
    /// revision is high-water: it survives remove so reload detection works
    EXPECT_EQ(env.clip_revision(kClipHash), 1u);

    env.add_clip(kClipHash, make_test_clip(0.2f));
    EXPECT_EQ(env.clip_revision(kClipHash), 2u);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, duplicate_clip_add_asserts) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));
    EXPECT_DEATH(env.add_clip(kClipHash, make_test_clip(0.05f)), ".*");
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, remove_clip_with_live_voice_asserts) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));
    const voice_id voice = env.start_voice(looping_params(kClipHash));
    ASSERT_NE(voice, 0u);
    EXPECT_DEATH(env.remove_clip(kClipHash), ".*");
    env.stop_voice(voice);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, clip_without_pcm_refuses_voice) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));

    audio_clip streamed{};
    streamed.streamed = true;
    streamed.source_absolute = "nonexistent.mp3";
    env.add_clip(kClipHash, std::move(streamed));

    EXPECT_EQ(env.start_voice(looping_params(kClipHash)), 0u);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, unknown_clip_refuses_voice) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    EXPECT_EQ(env.start_voice(looping_params(0xDEAD)), 0u);
    EXPECT_EQ(env.live_voice_count(), 0u);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, stop_voices_on_clip_leaves_others) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));
    env.add_clip(kOtherClipHash, make_test_clip(0.05f, 48000, 2));

    ASSERT_NE(env.start_voice(looping_params(kClipHash)), 0u);
    ASSERT_NE(env.start_voice(looping_params(kClipHash)), 0u);
    const voice_id other_voice = env.start_voice(looping_params(kOtherClipHash));
    ASSERT_NE(other_voice, 0u);
    EXPECT_EQ(env.live_voice_count(), 3u);

    env.stop_voices_on(kClipHash);
    EXPECT_EQ(env.live_voice_count(), 1u);
    EXPECT_FALSE(env.voice_finished(other_voice));

    env.stop_all_voices();
    env.remove_clip(kClipHash);
    env.remove_clip(kOtherClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, update_voice_and_listener_smoke) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.2f));

    voice_params params{};
    params.clip_hash = kClipHash;
    params.spatial = true;
    params.position = { 1.f, 2.f, 3.f };
    const voice_id voice = env.start_voice(params);
    ASSERT_NE(voice, 0u);

    env.set_listener({ 0.f, 0.f, 0.f }, glm::quat(1.f, 0.f, 0.f, 0.f), { 0.f, 0.f, 0.f });
    voice_dynamics dynamics{};
    dynamics.position = { 4.f, 5.f, 6.f };
    dynamics.velocity = { 1.f, 0.f, 0.f };
    dynamics.volume = 0.5f;
    dynamics.pitch = 1.5f;
    env.update_voice(voice, dynamics);

    env.pump(0.1);
    EXPECT_FALSE(env.voice_finished(voice));

    env.stop_voice(voice);
    env.remove_clip(kClipHash);
    env.shutdown();
  }

  TEST_F(audio_environment_tests, preview_plays_from_file_outside_pool) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.add_clip(kClipHash, make_test_clip(0.05f));

    /// fill the whole pool; the preview slot must still be available
    for (size_t i = 0; i < kMaxVoices; ++i) {
      ASSERT_NE(env.start_voice(looping_params(kClipHash)), 0u);
    }

    const filepath wav_dir = std::filesystem::temp_directory_path() / "other-audio-tests";
    std::filesystem::create_directories(wav_dir);
    const filepath wav_path = wav_dir / "preview_tone.wav";
    ASSERT_TRUE(write_test_wav(wav_path, 0.1f, 8000, 1));

    env.preview_play(wav_path, 0.8f);
    EXPECT_TRUE(env.preview_playing());

    env.pump(0.25);
    EXPECT_FALSE(env.preview_playing());

    env.preview_stop();
    env.preview_stop();  /// idempotent

    env.stop_all_voices();
    env.remove_clip(kClipHash);
    env.shutdown();
    std::filesystem::remove(wav_path);
  }

  TEST_F(audio_environment_tests, preview_missing_file_is_graceful) {
    audio_environment env;
    ASSERT_TRUE(env.initialize(pump_config()));
    env.preview_play("Z:/does/not/exist.wav");
    EXPECT_FALSE(env.preview_playing());
    env.shutdown();
  }

}  // namespace other
