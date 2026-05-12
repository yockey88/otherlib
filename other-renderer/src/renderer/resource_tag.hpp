/**
 * \file renderer/resource_tag.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP
#define OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP

#include "core/fnv.hpp"

namespace other {

  enum class resource_tag : uint64_t {
    NONE = 0,

    // buffer tags
    CAMERA = FNV("camera"),
    MODEL = FNV("model"),
    MATERIAL = FNV("material"),
    BONE = FNV("bone"),
    POINT_LIGHT = FNV("point_light"),
    DIRECTION_LIGHT = FNV("direction_light"),

    // texture tags
    SCREEN = FNV("screen"),
  };

  static inline resource_tag resource_tag_from_string(const std::string_view str) {
    if (str.empty()) return resource_tag::NONE;
    return static_cast<resource_tag>(FNV(str));
  }

  static inline std::string_view resource_tag_to_string(resource_tag tag) {
    switch (tag) {
      case resource_tag::CAMERA: return "camera";
      case resource_tag::MODEL: return "model";
      case resource_tag::MATERIAL: return "material";
      case resource_tag::BONE: return "bone";
      case resource_tag::POINT_LIGHT: return "point_light";
      case resource_tag::DIRECTION_LIGHT: return "direction_light";
      case resource_tag::SCREEN: return "screen";
      default: return "";
    }
  }

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP