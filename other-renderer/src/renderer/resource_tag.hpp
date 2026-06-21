/**
 * \file renderer/resource_tag.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP
#define OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP

#include <string>

#include "core/fnv.hpp"

#include "gpu_resource/texture.hpp"

namespace other {

  class resource_tag {
   public:
    constexpr resource_tag() = default;
    explicit constexpr resource_tag(natural_t h)
        : hash(h) {}
    resource_tag(resource_tag&&) = default;
    resource_tag& operator=(resource_tag&&) = default;
    resource_tag(const resource_tag&) = default;
    resource_tag& operator=(const resource_tag&) = default;
    resource_tag& operator=(natural_t h) {
      hash = h;
      return *this;
    }
    ~resource_tag() = default;

    static resource_tag from(const std::string_view str);
    static constexpr resource_tag none() { return resource_tag{ 0 }; }

    constexpr uint64_t value() const { return hash; }
    constexpr bool is_none() const { return hash == 0; }
    constexpr bool operator==(const resource_tag&) const = default;
    constexpr auto operator<=>(const resource_tag&) const = default;

    inline resource_tag resource_tag_from_string(std::string_view s) {
      return s.empty() ? resource_tag::none() : resource_tag::from(s);
    }

    // nice to have builtin
    constexpr static inline natural_t kCameraTag = FNV("camera");
    constexpr static inline natural_t kModelTag = FNV("model");
    constexpr static inline natural_t kMaterialTag = FNV("material");
    constexpr static inline natural_t kBoneTag = FNV("bone");
    constexpr static inline natural_t kPointLightTag = FNV("point_light");
    constexpr static inline natural_t kDirectionLightTag = FNV("direction_light");
    constexpr static inline natural_t kSimulationEnvironmentTag = FNV("simulation_environment");
    constexpr static inline natural_t kScreenTag = FNV("screen");

   private:
    natural_t hash = 0;
  };
  static_assert(sizeof(resource_tag) == sizeof(natural_t), "Resource tag should be light! size is too large");

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RESOURCE_TAG_HPP