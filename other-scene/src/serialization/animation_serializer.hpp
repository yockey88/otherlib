/**
 * \file serialization/animation_serializer.hpp
 *
 * .oanim — standalone binary animation clip:
 *
 *   [magic 'OANM' 4B] [format u16 = 1] [flags u16 = 0]
 *   [name str] [duration f32] [track_count u32]
 *   track_count ×:
 *     [joint_name_hash u64] [name str]
 *     [pos_count u32]   pos_count   × [t f32][vec3 12B]
 *     [rot_count u32]   rot_count   × [t f32][quat 16B, xyzw]
 *     [scale_count u32] scale_count × [t f32][vec3 12B]
 *
 * strings are [size u32][chars]; everything rides field_codec's raw little-endian
 * primitives. clips are homogeneous dense data, so there is no section table —
 * format bumps cover evolution. parsing never asserts on bytes: malformed input
 * is an error result.
 **/
#ifndef OTHER_SCENE_SERIALIZATION_ANIMATION_SERIALIZER_HPP
#define OTHER_SCENE_SERIALIZATION_ANIMATION_SERIALIZER_HPP

#include <span>

#include "core/defines.hpp"

#include "model/animation_clip.hpp"

namespace other {
  namespace serialization {

    constexpr inline std::string_view kAnimationClipExtension = ".oanim";

    struct clip_parse_result {
      opt<animation_clip> clip = std::nullopt;
      std::string error;
      bool success() const { return clip.has_value(); }
    };

    ostd::vector<uint8_t> serialize_animation_clip(const animation_clip& clip);
    clip_parse_result parse_animation_clip(std::span<const uint8_t> bytes);

    /// convenience for the asset loader and cli tools: read the file and parse (any thread)
    clip_parse_result load_animation_clip(const filepath& path);

  }  // namespace serialization
}  // namespace other

#endif  // OTHER_SCENE_SERIALIZATION_ANIMATION_SERIALIZER_HPP
