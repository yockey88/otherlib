/**
 * \file tests/audio/audio_import_tests.cpp
 *
 * pure decode/sidecar tests — no engine, no handler, standalone decoder only
 **/
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "audio/audio_import.hpp"

#include "audio/audio_test_fixtures.hpp"
#include "other_test.hpp"

namespace other {

  namespace {

    filepath import_test_dir() {
      const filepath dir = std::filesystem::temp_directory_path() / "other-audio-import-tests";
      std::filesystem::create_directories(dir);
      return dir;
    }

    void write_sidecar(const filepath& audio_path, const std::string_view body) {
      filepath sidecar = audio_path;
      sidecar += ".odecl.toml";
      std::ofstream out(sidecar);
      out << body;
    }

    void remove_sidecar(const filepath& audio_path) {
      filepath sidecar = audio_path;
      sidecar += ".odecl.toml";
      std::error_code ec;
      std::filesystem::remove(sidecar, ec);
    }

  }  // namespace

  class audio_import_tests : public other_test {};

  TEST_F(audio_import_tests, decode_wav_roundtrip) {
    const filepath wav = import_test_dir() / "stereo.wav";
    remove_sidecar(wav);
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 2));

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    EXPECT_TRUE(result.error.empty());
    EXPECT_EQ(result.clip->channels, 2u);
    EXPECT_EQ(result.clip->sample_rate, 8000u);
    EXPECT_EQ(result.clip->frames, 800u);
    EXPECT_EQ(result.clip->pcm.size(), 1600u);
    EXPECT_FALSE(result.clip->streamed);
    /// no sidecar: silent defaults
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_FALSE(result.clip->default_loop);
    EXPECT_FLOAT_EQ(result.clip->default_gain, 1.f);
  }

  TEST_F(audio_import_tests, garbage_header_fails_without_assert) {
    const filepath wav = import_test_dir() / "garbage.wav";
    remove_sidecar(wav);
    {
      std::ofstream out(wav, std::ios::binary);
      out << "this is not a riff container at all";
    }

    audio_import_result result = import_audio(wav);
    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.error.empty());
  }

  TEST_F(audio_import_tests, missing_file_fails_without_assert) {
    audio_import_result result = import_audio(import_test_dir() / "does-not-exist.wav");
    EXPECT_FALSE(result.success());
    EXPECT_FALSE(result.error.empty());
  }

  TEST_F(audio_import_tests, sidecar_full_roundtrip) {
    const filepath wav = import_test_dir() / "looped.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 1));
    write_sidecar(wav,
                  "schema-version = 1\n"
                  "[audio]\n"
                  "loop = true\n"
                  "loop-start = 0.025\n"
                  "loop-end = 0.075\n"
                  "volume = 0.8\n"
                  "stream = false\n");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_TRUE(result.clip->default_loop);
    EXPECT_FLOAT_EQ(result.clip->default_gain, 0.8f);
    EXPECT_EQ(result.clip->loop.start_frame, 200u);
    EXPECT_EQ(result.clip->loop.end_frame, 600u);
  }

  TEST_F(audio_import_tests, malformed_sidecar_defaults_with_warning) {
    const filepath wav = import_test_dir() / "badsidecar.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 1));
    write_sidecar(wav, "[audio\nloop = maybe");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    EXPECT_EQ(result.warnings.size(), 1u);
    EXPECT_FALSE(result.clip->default_loop);
    EXPECT_FLOAT_EQ(result.clip->default_gain, 1.f);
  }

  TEST_F(audio_import_tests, inverted_loop_region_swaps_with_warning) {
    const filepath wav = import_test_dir() / "inverted.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 1));
    write_sidecar(wav,
                  "[audio]\n"
                  "loop = true\n"
                  "loop-start = 0.075\n"
                  "loop-end = 0.025\n");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    ASSERT_EQ(result.warnings.size(), 1u);
    EXPECT_EQ(result.clip->loop.start_frame, 200u);
    EXPECT_EQ(result.clip->loop.end_frame, 600u);
  }

  TEST_F(audio_import_tests, out_of_range_volume_clamps_with_warning) {
    const filepath wav = import_test_dir() / "loud.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 1));
    write_sidecar(wav, "[audio]\nvolume = 5.0\n");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    ASSERT_EQ(result.warnings.size(), 1u);
    EXPECT_FLOAT_EQ(result.clip->default_gain, 2.f);
  }

  TEST_F(audio_import_tests, stream_flag_probes_metadata_only) {
    const filepath wav = import_test_dir() / "music.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.2f, 8000, 2));
    write_sidecar(wav, "[audio]\nstream = true\n");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    EXPECT_TRUE(result.clip->streamed);
    EXPECT_TRUE(result.clip->pcm.empty());
    EXPECT_EQ(result.clip->frames, 1600u);
    EXPECT_EQ(result.clip->channels, 2u);
    EXPECT_EQ(result.clip->source_absolute, wav);
  }

  TEST_F(audio_import_tests, newer_schema_version_warns_and_parses) {
    const filepath wav = import_test_dir() / "future.wav";
    ASSERT_TRUE(write_test_wav(wav, 0.1f, 8000, 1));
    write_sidecar(wav,
                  "schema-version = 2\n"
                  "[audio]\n"
                  "volume = 0.4\n");

    audio_import_result result = import_audio(wav);
    ASSERT_TRUE(result.success()) << result.error;
    ASSERT_EQ(result.warnings.size(), 1u);
    EXPECT_FLOAT_EQ(result.clip->default_gain, 0.4f);
  }

}  // namespace other
