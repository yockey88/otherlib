/**
 * \file audio/audio_import.hpp
 *
 * pure import helpers for the AUDIO asset pipeline: sidecar parsing and
 * decode/probe into an audio_clip. any-thread safe (standalone decoder per
 * call, never the engine); files are data — failures come back as results
 **/
#ifndef OTHER_AUDIO_AUDIO_AUDIO_IMPORT_HPP
#define OTHER_AUDIO_AUDIO_AUDIO_IMPORT_HPP

#include "core/defines.hpp"

#include "audio/audio_clip.hpp"

namespace other {

  /// <file>.odecl.toml beside the audio file; every field optional
  struct audio_sidecar {
    bool loop = false;
    double loop_start_seconds = 0.0;
    double loop_end_seconds = 0.0;  /// 0 => clip end
    float volume = 1.f;
    bool stream = false;
  };

  struct sidecar_parse_result {
    audio_sidecar sidecar;
    ostd::vector<std::string> warnings;
  };

  /// absent sidecar => silent defaults; malformed => defaults + warning —
  ///  the runtime never bricks an asset over a sidecar typo
  sidecar_parse_result parse_audio_sidecar(const filepath& audio_absolute);

  struct audio_import_result {
    opt<audio_clip> clip;
    std::string error;
    ostd::vector<std::string> warnings;

    bool success() const { return clip.has_value(); }
  };

  /// parses the sidecar, then decodes to interleaved f32 PCM at the source rate
  ///  (or probes metadata only when the sidecar flags stream = true)
  audio_import_result import_audio(const filepath& absolute);

}  // namespace other

#endif  // OTHER_AUDIO_AUDIO_AUDIO_IMPORT_HPP
