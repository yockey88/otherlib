/**
 * \file gpu_resource/renderer_resource.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_RENDERER_RESOURCE_HPP
#define OTHER_RENDERER_GPU_RESOURCE_RENDERER_RESOURCE_HPP

#include <cstdint>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  enum access_flags : uint32_t {
    READ = 0x1,
    WRITE = 0x2,
    READ_WRITE = READ | WRITE,
    NONE = 0x0
  };

  enum resource_type : uint8_t {
    IMAGE = 0,

    BUFFER,
    TEXTURE,
    SAMPLER,
    SHADER,
    MESH,
    FRAMEBUFFER,

    EMPTY,
    NUM_RESOURCES = EMPTY,
  };
  constexpr static size_t kNumResourceTypes = static_cast<size_t>(resource_type::NUM_RESOURCES);

  struct resource_handle {
    OTHER_REFLECTABLE(resource_handle);

    natural_t id = 0;
    natural_t name_hash = 0;
    resource_type type = resource_type::EMPTY;

    constexpr resource_handle() = default;
    constexpr resource_handle(natural_t id, resource_type type)
        : id(id), type(type) {}

    constexpr auto operator<=>(const resource_handle&) const = default;
  };

  class resource {
    OTHER_REFLECTABLE(resource);

   public:
    resource()
        : res_handle(0, resource_type::EMPTY) {}
    resource(resource_handle handle)
        : res_handle(handle) {}
    virtual ~resource() = default;

    virtual resource_type type() const { return res_handle.type; }

    const resource_handle& handle() const { return res_handle; }

    resource_handle res_handle;
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::resource_handle> : formatter<string_view> {
    template <typename FormatContext>
    auto format(const other::resource_handle& handle, FormatContext& ctx) const {
      return formatter<string_view>::format(std::format("[{}:{}]", handle.id, handle.type), ctx);
    }
  };

}  // namespace std

OTHER_REFLECT(
  other::access_flags
)

OTHER_REFLECT(
  other::resource_type
)

OTHER_REFLECT(
  other::resource_handle,
  field(id, other::attr::serializable()),
  field(name_hash, other::attr::serializable()),
  field(type, other::attr::serializable())
)

OTHER_REFLECT(
  other::resource,
  field(res_handle, other::attr::serializable())
)

#endif  // OTHER_RENDERER_RESOURCE_RENDERER_RESOURCE_HPP