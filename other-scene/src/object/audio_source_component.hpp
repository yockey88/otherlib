/**
 * \file object/audio_source_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_AUDIO_SOURCE_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_AUDIO_SOURCE_COMPONENT_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "asset/asset.hpp"

namespace other {

  /// describes what should be audible on this object; the audio system reconciles
  ///  desired state against live voices every tick — components never own voices
  struct audio_source_component {
    natural_t clip_asset_id = 0;  /// AUDIO asset; portable path in .oscn via the codec

    bool playing = false;  /// desired state; authored true == autoplay on scene play.
                           ///  flipped back to false when a non-looping voice finishes
                           ///  (the pollable completion signal for scripts)
    bool looping = false;
    float volume = 1.f;  /// multiplier over the clip's sidecar default gain
    float pitch = 1.f;
    uint32_t bus = 2;  /// audio_bus: MASTER=0 MUSIC=1 SFX=2 UI=3 (u32: the script
                       ///  field ABI has no u8 lane)

    bool spatial = true;  /// false => 2D (ui/music) — no attenuation or positioning
    float min_distance = 1.f;
    float max_distance = 500.f;
    float doppler_factor = 1.f;  /// 0 disables

    /// runtime state, never reflected: rebuilt by the reconciler whenever identity
    ///  changes, so codec restores and hot reloads need no ceremony. voice_id is a
    ///  core alias — no audio include here
    voice_id voice = 0;
    natural_t bound_clip_id = 0;
    uint32_t bound_clip_revision = 0;
    glm::vec3 prev_position{ 0.f };  /// finite-difference velocity; reseeded on bind
  };

}  // namespace other

OTHER_REFLECT(
  other::audio_source_component,
  field(clip_asset_id, other::attr::serializable("Clip"),
        other::attr::asset_identifier_field(other::asset::AUDIO)),
  field(playing, other::attr::serializable("Playing")),
  field(looping, other::attr::serializable("Looping")),
  field(volume, other::attr::serializable("Volume")),
  field(pitch, other::attr::serializable("Pitch")),
  field(bus, other::attr::serializable("Bus")),
  field(spatial, other::attr::serializable("Spatial")),
  field(min_distance, other::attr::serializable("Min Distance")),
  field(max_distance, other::attr::serializable("Max Distance")),
  field(doppler_factor, other::attr::serializable("Doppler")))

#endif  // OTHER_SCENE_OBJECT_AUDIO_SOURCE_COMPONENT_HPP
