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
    CUBEMAP,
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

  struct resource {
    resource()
        : res_handle(0, resource_type::EMPTY) {}
    resource(resource_handle handle)
        : res_handle(handle) {}
    virtual ~resource() = default;
    virtual resource_type type() const { return res_handle.type; }

    // const std::string get_name() const { return name; }
    const resource_handle& handle() const { return res_handle; }

    std::string name;
    resource_handle res_handle;
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::resource_handle> : formatter<string_view> {
    template <typename FormatContext>
    auto format(const other::resource_handle& handle, FormatContext& ctx) const {
      using namespace std::string_view_literals;
      constexpr std::string_view fmt_str = "[{}:{}]"sv;
      return format_to(ctx.out(), fmt_str, handle.type, handle.id);
    }
  };

}  // namespace std

#endif  // OTHER_RENDERER_RESOURCE_RENDERER_RESOURCE_HPP