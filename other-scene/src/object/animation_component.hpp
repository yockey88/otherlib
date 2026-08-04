/**
 * \file object/animation_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_ANIMATION_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_ANIMATION_COMPONENT_HPP

#include <string>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "model/pose.hpp"

#include "asset/asset.hpp"

namespace other {

  /// plays one clip on the owning object's rigged model; clips are immutable shared
  ///  data, all playback state lives here
  struct animation_component {
    natural_t last_animation_asset_id = 0;
    natural_t animation_asset_id = 0;  /// standalone .oanim asset; 0 => clip_name against the model's embedded clips
    std::string clip_name;

    bool playing = true;
    bool looping = true;
    float speed = 1.f;
    float time = 0.f;  /// seconds; serialized so snapshots restore mid-pose

    /// runtime state, never reflected: rebuilt by the tick whenever the resolved clip or
    ///  skeleton identity changes, so codec restores and hot reloads need no ceremony
    const animation_clip* clip = nullptr;
    const skeleton* bound_skeleton = nullptr;
    clip_binding binding;
    pose working_pose;
  };

}  // namespace other

OTHER_REFLECT(
  other::animation_component,
  field(animation_asset_id, other::attr::serializable("Clip"),
        other::attr::asset_identifier_field(other::asset::ANIMATION)),
  field(clip_name, other::attr::serializable("Embedded Clip")),
  field(playing, other::attr::serializable("Playing")),
  field(looping, other::attr::serializable("Looping")),
  field(speed, other::attr::serializable("Speed")),
  field(time, other::attr::serializable("Time")))

#endif  // OTHER_SCENE_OBJECT_ANIMATION_COMPONENT_HPP
