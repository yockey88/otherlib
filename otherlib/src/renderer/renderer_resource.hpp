/**
 * \file renderer/renderer_resource.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RESOURCE_HPP
#define OTHER_RENDERER_RENDERER_RESOURCE_HPP

#include <compare>
#include <cstdint>
#include <string>

#include "core/defines.hpp"

namespace other {

  enum access_flags : uint32_t {
    READ = 0x1,
    WRITE = 0x2,
    READ_WRITE = READ | WRITE,
    NONE = 0x0
  };

  enum class resource_type {
    IMAGE = 0,

    BUFFER,
    TEXTURE,
    SAMPLER,

    SHADER,

    MESH,

    EMPTY,
    NUM_RESOURCES = EMPTY,
  };
  constexpr static size_t kNumResourceTypes = static_cast<size_t>(resource_type::NUM_RESOURCES);

  struct resource_handle {
    uint64_t id = 0;
    uint64_t name_hash = 0;
    resource_type type = resource_type::EMPTY;

    constexpr resource_handle() = default;
    constexpr resource_handle(uint64_t id, resource_type type)
        : id(id), type(type) {}

    constexpr auto operator<=>(const resource_handle&) const = default;
  };

  class resource {
   public:
    resource(resource_handle handle)
        : res_handle(handle) {}
    virtual ~resource() = default;

    virtual resource_type type() const = 0;

    const resource_handle& handle() const { return res_handle; }

   private:
    resource_handle res_handle;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RESOURCE_HPP