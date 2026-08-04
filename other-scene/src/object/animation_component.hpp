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

  /// plays one clip on the owning object's rigged model. clips are immutable shared
  ///  data — ALL playback state lives here (doc 03 §5). when a doc-04 graph component
  ///  targets the pose it owns resolve/sample/palette and this becomes the output target
  struct animation_component {
    natural_t last_animation_asset_id = 0;
    natural_t animation_asset_id = 0;  /// standalone .oanim asset; 0 => clip_name against the model's embedded clips
    std::string clip_name;             /// embedded-clip lookup on this object's model_source

    bool playing = true;
    bool looping = true;
    float speed = 1.f;
    float time = 0.f;  /// seconds; serialized => snapshots restore mid-pose (play/stop just works)

    /// runtime state, rebuilt whenever the resolved clip / skeleton identity changes
    ///  (hot-reload safe by revalidation, the obj_model pattern); never reflected —
    ///  codec restores leave these null and the next tick rebuilds them
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
