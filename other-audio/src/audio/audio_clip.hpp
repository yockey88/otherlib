/**
 * \file audio/audio_clip.hpp
 **/
#ifndef OTHER_AUDIO_AUDIO_AUDIO_CLIP_HPP
#define OTHER_AUDIO_AUDIO_AUDIO_CLIP_HPP

#include "core/defines.hpp"

namespace other {

  struct loop_region {
    uint64_t start_frame = 0;
    uint64_t end_frame = 0;  /// 0 => clip end
  };

  /// immutable shared data; all playback state lives on voices. exactly one of
  ///  {pcm non-empty, streamed} holds after a successful load
  struct audio_clip {
    uint32_t channels = 0;
    uint32_t sample_rate = 0;  /// source rate; the engine resamples at mix time
    uint64_t frames = 0;
    float seconds = 0.f;

    /// sidecar-supplied, applied at voice start — never baked into samples
    float default_gain = 1.f;
    bool default_loop = false;
    loop_region loop;

    /// decoded variant: interleaved f32; voices wrap this in per-voice buffer
    ///  views, so simultaneous voices share one allocation
    ostd::vector<float> pcm;

    /// streamed variant: voices open from this path through the resource manager
    bool streamed = false;
    filepath source_absolute;
  };

}  // namespace other

#endif  // OTHER_AUDIO_AUDIO_AUDIO_CLIP_HPP
