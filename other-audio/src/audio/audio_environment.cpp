/**
 * \file audio/audio_environment.cpp
 **/
#include "audio/audio_environment.hpp"

#include <array>

#include <miniaudio/miniaudio.h>

#include "core/logger.hpp"
#include "memory/arena_allocator.hpp"
#include "thread/thread_safety.hpp"

namespace other {

  namespace {

    constexpr size_t kPumpChunkFrames = 512;
    constexpr uint32_t kPumpChannels = 2;

    voice_id encode_voice(size_t slot_index, uint16_t generation) {
      return (static_cast<voice_id>(generation) << 16) | static_cast<voice_id>(slot_index + 1);
    }

  }  // namespace

  struct audio_environment::impl {
    ma_engine engine{};

    /// MUSIC / SFX / UI route through groups; MASTER is the engine's own volume
    std::array<ma_sound_group, kNumAudioBuses - 1> groups{};
    bool groups_live = false;

    struct voice_slot {
      bool in_use = false;
      uint16_t generation = 1;
      natural_t clip_hash = 0;
      bool looping = false;
      bool one_shot = false;
      bool streamed = false;  /// no buffer view; the sound owns a file stream
      ma_audio_buffer buffer{};
      ma_sound sound{};
    };
    std::array<voice_slot, kMaxVoices> slots{};

    bool preview_live = false;
    ma_sound preview_sound{};

    double frame_accumulator = 0.0;
    std::array<float, kPumpChunkFrames * kPumpChannels> pump_scratch{};

    ma_sound_group* group_for(audio_bus bus) {
      if (bus == audio_bus::MASTER || bus >= audio_bus::NUM_BUSES) {
        return nullptr;  /// engine endpoint
      }
      return &groups[static_cast<size_t>(bus) - 1];
    }

    voice_slot* resolve(voice_id id) {
      const size_t index = (id & 0xFFFF);
      if (index == 0 || index > kMaxVoices) {
        return nullptr;
      }
      voice_slot& slot = slots[index - 1];
      if (!slot.in_use || slot.generation != static_cast<uint16_t>(id >> 16)) {
        return nullptr;
      }
      return &slot;
    }

    void kill_slot(voice_slot& slot) {
      ma_sound_uninit(&slot.sound);
      if (!slot.streamed) {
        ma_audio_buffer_uninit(&slot.buffer);
      }
      slot.in_use = false;
      slot.clip_hash = 0;
      slot.looping = false;
      slot.one_shot = false;
      slot.streamed = false;
      slot.generation++;
      if (slot.generation == 0) {
        slot.generation = 1;
      }
    }
  };

  audio_environment::~audio_environment() {
    shutdown();
  }

  /// no thread assert here: subsystem initialization runs before the driver
  ///  registers the main thread (other.cpp boot order), same as the renderer
  bool audio_environment::initialize(const audio_config& config) {
    if (initialized) {
      CORE_LOG_WARN("audio_environment::initialize called while already initialized; ignoring");
      return true;
    }

    bool want_pump = config.force_pump_mode;
    engine_sample_rate = config.sample_rate;
    if (engine_sample_rate == 0) {
      CORE_LOG_WARN("audio.sample-rate 0 is invalid; falling back to pump mode at 48000 Hz");
      engine_sample_rate = 48000;
      want_pump = true;
    }

    state = arena_allocator<impl>{}.allocate();
    OTHER_ASSERT(state != nullptr, "Failed to allocate audio environment state");

    ma_engine_config engine_config = ma_engine_config_init();
    engine_config.channels = kPumpChannels;
    engine_config.sampleRate = engine_sample_rate;
    engine_config.noDevice = want_pump ? MA_TRUE : MA_FALSE;

    ma_result result = ma_engine_init(&engine_config, &state->engine);
    if (result != MA_SUCCESS && !want_pump) {
      CORE_LOG_WARN("Audio device open failed ({}); falling back to silent pump mode", static_cast<int>(result));
      want_pump = true;
      engine_config.noDevice = MA_TRUE;
      result = ma_engine_init(&engine_config, &state->engine);
    }
    if (result != MA_SUCCESS) {
      CORE_LOG_ERROR("Audio engine initialization failed ({}); audio is unavailable", static_cast<int>(result));
      arena_allocator<impl>{}.free(state);
      state = nullptr;
      return false;
    }

    state->groups_live = true;
    for (size_t i = 0; i < state->groups.size(); ++i) {
      const ma_result group_result = ma_sound_group_init(&state->engine, 0, nullptr, &state->groups[i]);
      if (group_result != MA_SUCCESS) {
        CORE_LOG_ERROR("Audio bus group {} initialization failed ({})", i + 1, static_cast<int>(group_result));
        state->groups_live = false;
      }
    }

    initialized = true;
    pump_active = want_pump;
    for (size_t i = 0; i < kNumAudioBuses; ++i) {
      set_bus_volume(static_cast<audio_bus>(i), bus_volumes[i]);
    }

    if (pump_active) {
      CORE_LOG_INFO("Audio engine live in pump mode ({} Hz, no device)", engine_sample_rate);
    } else {
      CORE_LOG_INFO("Audio engine live ({} Hz)", ma_engine_get_sample_rate(&state->engine));
    }
    return true;
  }

  void audio_environment::shutdown() {
    if (!initialized) {
      return;
    }
    ASSERT_MAIN_THREAD();

    stop_all_voices();

    if (state->groups_live) {
      for (ma_sound_group& group : state->groups) {
        ma_sound_group_uninit(&group);
      }
      state->groups_live = false;
    }

    ma_engine_uninit(&state->engine);
    arena_allocator<impl>{}.free(state);
    state = nullptr;

    clips.clear();
    initialized = false;
    pump_active = false;
  }

  void audio_environment::pump(double dt) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(initialized && pump_active, "pump() is only valid on an initialized pump-mode audio environment");
    if (dt <= 0.0) {
      return;
    }

    state->frame_accumulator += dt * static_cast<double>(engine_sample_rate);
    uint64_t frames = static_cast<uint64_t>(state->frame_accumulator);
    state->frame_accumulator -= static_cast<double>(frames);

    while (frames > 0) {
      const uint64_t chunk = std::min<uint64_t>(frames, kPumpChunkFrames);
      ma_engine_read_pcm_frames(&state->engine, state->pump_scratch.data(), chunk, nullptr);
      frames -= chunk;
    }
  }

  void audio_environment::add_clip(natural_t path_hash, audio_clip&& clip) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(clips.find(path_hash) == clips.end(),
                 "Duplicate audio clip registration for hash {:#x} — refresh must unload first", path_hash);
    clips.emplace(path_hash, std::move(clip));
    clip_revisions[path_hash]++;
  }

  const audio_clip* audio_environment::get_clip(natural_t path_hash) const {
    const auto it = clips.find(path_hash);
    return it != clips.end() ? &it->second : nullptr;
  }

  uint32_t audio_environment::clip_revision(natural_t path_hash) const {
    const auto it = clip_revisions.find(path_hash);
    return it != clip_revisions.end() ? it->second : 0;
  }

  void audio_environment::remove_clip(natural_t path_hash) {
    ASSERT_MAIN_THREAD();
    if (initialized) {
      for (const impl::voice_slot& slot : state->slots) {
        OTHER_ASSERT(!slot.in_use || slot.clip_hash != path_hash,
                     "remove_clip({:#x}) while a voice still reads it — stop_voices_on first", path_hash);
      }
    }

    if (clips.erase(path_hash) == 0) {
      CORE_LOG_ERROR("remove_clip: no clip registered for hash {:#x}", path_hash);
    }
  }

  voice_id audio_environment::start_voice(const voice_params& params) {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      CORE_LOG_ERROR("start_voice on an uninitialized audio environment");
      return 0;
    }

    const audio_clip* clip = get_clip(params.clip_hash);
    if (clip == nullptr) {
      CORE_LOG_WARN("start_voice: no clip registered for hash {:#x}", params.clip_hash);
      return 0;
    }
    if (!clip->streamed && clip->pcm.empty()) {
      CORE_LOG_WARN("start_voice: clip {:#x} carries no decoded PCM", params.clip_hash);
      return 0;
    }

    impl::voice_slot* free_slot = nullptr;
    size_t slot_index = 0;
    for (size_t i = 0; i < state->slots.size(); ++i) {
      if (!state->slots[i].in_use) {
        free_slot = &state->slots[i];
        slot_index = i;
        break;
      }
    }
    if (free_slot == nullptr) {
      CORE_LOG_WARN("Voice pool exhausted ({} voices); refusing new voice", kMaxVoices);
      return 0;
    }

    if (clip->streamed) {
      const ma_uint32 flags = MA_SOUND_FLAG_STREAM | (params.spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION);
      const std::string path = clip->source_absolute.string();
      if (ma_sound_init_from_file(&state->engine, path.c_str(), flags,
                                  state->group_for(params.bus), nullptr, &free_slot->sound) != MA_SUCCESS) {
        CORE_LOG_ERROR("start_voice: stream open failed for clip {:#x} ('{}')", params.clip_hash, path);
        return 0;
      }
      free_slot->streamed = true;
    } else {
      ma_audio_buffer_config buffer_config =
        ma_audio_buffer_config_init(ma_format_f32, clip->channels, clip->frames, clip->pcm.data(), nullptr);
      buffer_config.sampleRate = clip->sample_rate;
      if (ma_audio_buffer_init(&buffer_config, &free_slot->buffer) != MA_SUCCESS) {
        CORE_LOG_ERROR("start_voice: audio buffer init failed for clip {:#x}", params.clip_hash);
        return 0;
      }

      const ma_uint32 flags = params.spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
      if (ma_sound_init_from_data_source(&state->engine, &free_slot->buffer, flags,
                                         state->group_for(params.bus), &free_slot->sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&free_slot->buffer);
        CORE_LOG_ERROR("start_voice: sound init failed for clip {:#x}", params.clip_hash);
        return 0;
      }
      free_slot->streamed = false;
    }

    ma_sound_set_volume(&free_slot->sound, clip->default_gain * params.volume);
    ma_sound_set_pitch(&free_slot->sound, params.pitch);
    ma_sound_set_looping(&free_slot->sound, params.looping ? MA_TRUE : MA_FALSE);
    if (params.looping && clip->loop.end_frame > clip->loop.start_frame) {
      /// sidecar loop region; whole-clip loop otherwise
      ma_data_source* source = ma_sound_get_data_source(&free_slot->sound);
      if (source != nullptr) {
        ma_data_source_set_loop_point_in_pcm_frames(source, clip->loop.start_frame, clip->loop.end_frame);
      }
    }
    if (params.spatial) {
      ma_sound_set_position(&free_slot->sound, params.position.x, params.position.y, params.position.z);
      ma_sound_set_min_distance(&free_slot->sound, params.min_distance);
      ma_sound_set_max_distance(&free_slot->sound, params.max_distance);
      ma_sound_set_attenuation_model(&free_slot->sound, ma_attenuation_model_inverse);
      ma_sound_set_doppler_factor(&free_slot->sound, params.doppler_factor);
    }
    ma_sound_start(&free_slot->sound);

    free_slot->in_use = true;
    free_slot->clip_hash = params.clip_hash;
    free_slot->looping = params.looping;
    free_slot->one_shot = false;
    return encode_voice(slot_index, free_slot->generation);
  }

  void audio_environment::play_one_shot(const voice_params& params) {
    const voice_id id = start_voice(params);
    if (id == 0) {
      return;
    }
    const size_t index = (id & 0xFFFF) - 1;
    state->slots[index].one_shot = true;
  }

  void audio_environment::recycle_finished_one_shots() {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      return;
    }
    for (impl::voice_slot& slot : state->slots) {
      if (slot.in_use && slot.one_shot && !slot.looping && ma_sound_at_end(&slot.sound) != MA_FALSE) {
        state->kill_slot(slot);
      }
    }
  }

  void audio_environment::stop_voice(voice_id id) {
    ASSERT_MAIN_THREAD();
    impl::voice_slot* slot = initialized ? state->resolve(id) : nullptr;
    OTHER_ASSERT(slot != nullptr, "stop_voice on unknown voice id {:#x}", id);
    state->kill_slot(*slot);
  }

  void audio_environment::update_voice(voice_id id, const voice_dynamics& dynamics) {
    ASSERT_MAIN_THREAD();
    impl::voice_slot* slot = initialized ? state->resolve(id) : nullptr;
    OTHER_ASSERT(slot != nullptr, "update_voice on unknown voice id {:#x}", id);

    const audio_clip* clip = get_clip(slot->clip_hash);
    const float clip_gain = clip != nullptr ? clip->default_gain : 1.f;
    ma_sound_set_volume(&slot->sound, clip_gain * dynamics.volume);
    ma_sound_set_pitch(&slot->sound, dynamics.pitch);
    ma_sound_set_position(&slot->sound, dynamics.position.x, dynamics.position.y, dynamics.position.z);
    ma_sound_set_velocity(&slot->sound, dynamics.velocity.x, dynamics.velocity.y, dynamics.velocity.z);
  }

  bool audio_environment::voice_alive(voice_id id) const {
    return initialized && id != 0 && state->resolve(id) != nullptr;
  }

  bool audio_environment::voice_finished(voice_id id) const {
    impl::voice_slot* slot = initialized ? state->resolve(id) : nullptr;
    OTHER_ASSERT(slot != nullptr, "voice_finished on unknown voice id {:#x}", id);
    if (slot->looping) {
      return false;
    }
    return ma_sound_at_end(&slot->sound) != MA_FALSE;
  }

  void audio_environment::stop_voices_on(natural_t clip_hash) {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      return;
    }
    for (impl::voice_slot& slot : state->slots) {
      if (slot.in_use && slot.clip_hash == clip_hash) {
        state->kill_slot(slot);
      }
    }
  }

  void audio_environment::stop_all_voices() {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      return;
    }
    for (impl::voice_slot& slot : state->slots) {
      if (slot.in_use) {
        state->kill_slot(slot);
      }
    }
    preview_stop();
  }

  size_t audio_environment::live_voice_count() const {
    if (!initialized) {
      return 0;
    }
    size_t count = 0;
    for (const impl::voice_slot& slot : state->slots) {
      count += slot.in_use ? 1 : 0;
    }
    return count;
  }

  void audio_environment::set_listener(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& velocity) {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      return;
    }
    const glm::vec3 forward = rotation * glm::vec3(0.f, 0.f, -1.f);
    const glm::vec3 up = rotation * glm::vec3(0.f, 1.f, 0.f);
    ma_engine_listener_set_position(&state->engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&state->engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&state->engine, 0, up.x, up.y, up.z);
    ma_engine_listener_set_velocity(&state->engine, 0, velocity.x, velocity.y, velocity.z);
  }

  void audio_environment::set_bus_volume(audio_bus bus, float volume) {
    OTHER_ASSERT(bus < audio_bus::NUM_BUSES, "Invalid audio bus {}", static_cast<uint32_t>(bus));
    bus_volumes[static_cast<size_t>(bus)] = volume;
    if (!initialized) {
      return;
    }
    if (bus == audio_bus::MASTER) {
      ma_engine_set_volume(&state->engine, volume);
    } else if (state->groups_live) {
      ma_sound_group_set_volume(state->group_for(bus), volume);
    }
  }

  float audio_environment::bus_volume(audio_bus bus) const {
    OTHER_ASSERT(bus < audio_bus::NUM_BUSES, "Invalid audio bus {}", static_cast<uint32_t>(bus));
    return bus_volumes[static_cast<size_t>(bus)];
  }

  void audio_environment::preview_play(const filepath& absolute, float volume, opt<loop_region> loop) {
    ASSERT_MAIN_THREAD();
    if (!initialized) {
      CORE_LOG_WARN("preview_play on an uninitialized audio environment");
      return;
    }
    preview_stop();

    /// synchronous decode keeps preview deterministic in pump mode
    const ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
    const std::string path = absolute.string();
    if (ma_sound_init_from_file(&state->engine, path.c_str(), flags, nullptr, nullptr, &state->preview_sound) != MA_SUCCESS) {
      CORE_LOG_WARN("preview_play: failed to open '{}'", path);
      return;
    }
    state->preview_live = true;

    ma_sound_set_volume(&state->preview_sound, volume);
    if (loop.has_value()) {
      ma_data_source* source = ma_sound_get_data_source(&state->preview_sound);
      if (source != nullptr && loop->end_frame > loop->start_frame) {
        ma_data_source_set_loop_point_in_pcm_frames(source, loop->start_frame, loop->end_frame);
      }
      ma_sound_set_looping(&state->preview_sound, MA_TRUE);
    }
    ma_sound_start(&state->preview_sound);
  }

  void audio_environment::preview_stop() {
    ASSERT_MAIN_THREAD();
    if (!initialized || !state->preview_live) {
      return;
    }
    ma_sound_uninit(&state->preview_sound);
    state->preview_live = false;
  }

  bool audio_environment::preview_playing() const {
    if (!initialized || !state->preview_live) {
      return false;
    }
    if (ma_sound_is_looping(&state->preview_sound)) {
      return true;
    }
    return ma_sound_at_end(&state->preview_sound) == MA_FALSE;
  }

}  // namespace other
