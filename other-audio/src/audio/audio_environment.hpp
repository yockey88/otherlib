/**
 * \file audio/audio_environment.hpp
 *
 * inert-gated audio capability + state owner: the miniaudio engine, the clip
 * registry, the voice pool, and the bus set. all calls are main-thread; the
 * device callback thread is miniaudio-internal and never touches engine state.
 * miniaudio itself is pimpl'd out of this header — consumers never pay for it.
 **/
#ifndef OTHER_AUDIO_AUDIO_AUDIO_ENVIRONMENT_HPP
#define OTHER_AUDIO_AUDIO_AUDIO_ENVIRONMENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/defines.hpp"
#include "core/subsystem.hpp"

#include "audio/audio_clip.hpp"

namespace other {

  /// components + one-shots + nothing else; the preview slot lives outside this cap
  constexpr inline size_t kMaxVoices = 64;

  enum class audio_bus : uint8_t {
    MASTER = 0,
    MUSIC,
    SFX,
    UI,

    NUM_BUSES,
  };
  constexpr inline size_t kNumAudioBuses = static_cast<size_t>(audio_bus::NUM_BUSES);

  struct audio_config {
    uint32_t sample_rate = 48000;
    bool force_pump_mode = false;  /// config key audio.force-pump — headless/CI/tests
  };

  struct voice_params {
    natural_t clip_hash = 0;
    float volume = 1.f;  /// multiplied with the clip's default_gain at start
    float pitch = 1.f;
    bool looping = false;
    audio_bus bus = audio_bus::SFX;

    bool spatial = true;
    float min_distance = 1.f;
    float max_distance = 500.f;
    float doppler_factor = 1.f;
    glm::vec3 position{ 0.f };
  };

  struct voice_dynamics {
    glm::vec3 position{ 0.f };
    glm::vec3 velocity{ 0.f };
    float volume = 1.f;  /// same clip-gain multiplication as voice_params::volume
    float pitch = 1.f;
  };

  class audio_environment : public subsystem<audio_environment> {
   public:
    audio_environment() = default;
    virtual ~audio_environment();

    /// device open; any failure (or force/invalid config) falls back to pump mode
    ///  and warns once — the environment is always usable after this returns true
    bool initialize(const audio_config& config);
    void shutdown();

    bool is_initialized() const { return initialized; }
    bool pump_mode() const { return pump_active; }
    /// pump mode only: advance the engine by dt worth of frames into a discard
    ///  buffer — deterministic time for CI/tests; called by audio_system per tick
    void pump(double dt);

    /// clip registry — keyed by asset path_hash; add asserts duplicate keys (refresh
    ///  is unload-then-load), get-miss is a value, remove asserts no live voices
    ///  still read the clip and logs on missing keys; revisions are high-water and
    ///  survive remove so consumers can detect hot reloads
    void add_clip(natural_t path_hash, audio_clip&& clip);
    const audio_clip* get_clip(natural_t path_hash) const;
    uint32_t clip_revision(natural_t path_hash) const;
    void remove_clip(natural_t path_hash);

    /// voices — ids encode slot+generation; 0 == pool exhausted (warn, not assert);
    ///  every other misuse of an id is a programmer error and asserts
    voice_id start_voice(const voice_params& params);
    void stop_voice(voice_id id);
    void update_voice(voice_id id, const voice_dynamics& dynamics);
    bool voice_finished(voice_id id) const;
    void stop_voices_on(natural_t clip_hash);
    void stop_all_voices();
    size_t live_voice_count() const;

    void set_listener(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& velocity);

    void set_bus_volume(audio_bus bus, float volume);
    float bus_volume(audio_bus bus) const;

    /// edit-mode audition (editor transport): one reserved slot outside kMaxVoices,
    ///  decoded synchronously from the file, ignored by scene reconciliation
    void preview_play(const filepath& absolute, float volume = 1.f, opt<loop_region> loop = std::nullopt);
    void preview_stop();
    bool preview_playing() const;

   private:
    struct impl;
    impl* state = nullptr;

    bool initialized = false;
    bool pump_active = false;
    uint32_t engine_sample_rate = 48000;

    ostd::map<natural_t, audio_clip> clips;
    ostd::map<natural_t, uint32_t> clip_revisions;
    float bus_volumes[kNumAudioBuses] = { 1.f, 1.f, 1.f, 1.f };
  };

}  // namespace other

OTHER_DEPENDENT_SUBSYSTEM(
  other::audio_environment,
  subsystem_profile::kArena,
  subsystem_profile::kLogger);

#endif  // OTHER_AUDIO_AUDIO_AUDIO_ENVIRONMENT_HPP
