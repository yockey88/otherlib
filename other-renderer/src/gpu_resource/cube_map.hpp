/**
 * \file gpu_resource/cube_map.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_CUBE_MAP_HPP
#define OTHER_RENDERER_GPU_RESOURCE_CUBE_MAP_HPP

#include <cstdint>

#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/texture.hpp"

namespace other {

  class cube_map : public resource {
   public:
    enum face : uint32_t {
      POSITIVE_X = 0,
      NEGATIVE_X,
      POSITIVE_Y,
      NEGATIVE_Y,
      POSITIVE_Z,
      NEGATIVE_Z,

      NUM_FACES
    };

    cube_map() = default;
    cube_map(resource_handle handle)
        : resource(handle) {}
    virtual ~cube_map() = default;

    resource_type type() const override { return resource_type::CUBEMAP; }

    static resource_handle create(const std::string& name, texture::format format, uint32_t width, uint32_t height);

    cube_map& bind(uint32_t slot = 0);
    cube_map& set_face_size(uint32_t width, uint32_t height);
    cube_map& set_format(texture::format frmt);
    cube_map& set_filter(texture::filter min_filter, texture::filter mag_filter = texture::NEAREST);
    cube_map& set_wrap_mode(texture::wrap wrap_s, texture::wrap wrap_t = texture::CLAMP_TO_EDGE, texture::wrap wrap_r = texture::CLAMP_TO_EDGE);
    cube_map& set_data(face face_idx, const std::vector<uint8_t>& data);
    cube_map& set_data(face face_idx, const uint8_t* data, size_t size);

    void unbind(uint32_t slot = 0);
    void finalize_cube_map();

    constexpr static inline size_t kCubeFaces = 6;

   private:
    std::array<std::vector<uint8_t>, kCubeFaces> faces;

    uint32_t mip_level = 0;
    glm::ivec2 face_size = { 0, 0 };
    texture::format texture_format = texture::format::RGBA8;

    texture::filter min_filter = texture::filter::LINEAR;
    texture::filter mag_filter = texture::filter::LINEAR;

    texture::wrap wrap_s = texture::wrap::CLAMP_TO_EDGE;
    texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE;
    texture::wrap wrap_r = texture::wrap::CLAMP_TO_EDGE;
  };

}  // namespace other

#endif  // OTHER_RENDERER_GPU_RESOURCE_CUBE_MAP_HPP