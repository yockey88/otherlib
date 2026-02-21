/**
 * \file renderer/texture.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_TEXTURE_HPP
#define OTHER_RENDERER_GPU_RESOURCE_TEXTURE_HPP

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "math/definitions.hpp"
#include "serialization/reflection.hpp"

#include "gpu_resource/renderer_resource.hpp"

namespace other {

  struct texture : public resource {
    enum tex_type : uint32_t {
      TEXTURE_1D = 0,
      TEXTURE_2D,
      TEXTURE_3D,

      TEXTURE_CUBE,
      TEXTURE_CUBE_FACE_POSITIVE_X,
      TEXTURE_CUBE_FACE_NEGATIVE_X,
      TEXTURE_CUBE_FACE_POSITIVE_Y,
      TEXTURE_CUBE_FACE_NEGATIVE_Y,
      TEXTURE_CUBE_FACE_POSITIVE_Z,
      TEXTURE_CUBE_FACE_NEGATIVE_Z,

      /// add more here...

      NUM_TEXTURE_TYPES
    };

    /**
     * \note the format here is a less inclusive version of bgfx's, see https://github.com/bkaradzic/bgfx/blob/master/include/bgfx/bgfx.h line 139-265
     * format:
          RGBA16S
          ^   ^ ^
          |   | |
          |   | +-- [ ]Unorm
          |   |     [F]loat
          |   |     [S]norm
          |   |     [I]nt
          |   |     [U]int
          |   +---- Number of bits per component
          +-------- Components
     **/
    enum format : uint32_t {
      R1 = 0,
      A8,
      R8,
      R8I,
      R8U,
      R8S,
      R16,
      R16I,
      R16U,
      R16F,
      R16S,
      R32I,
      R32U,
      R32F,
      RG8,
      RG8I,
      RG8U,
      RG8S,
      RG16,
      RG16I,
      RG16U,
      RG16F,
      RG16S,
      RG32I,
      RG32U,
      RG32F,
      RGB8,
      RGB8I,
      RGB8U,
      RGB8S,
      RGB9E5F,
      BGRA8,
      RGBA8,
      RGBA8I,
      RGBA8U,
      RGBA8S,
      RGBA16,
      RGBA16I,
      RGBA16U,
      RGBA16F,
      RGBA16S,
      RGBA32I,
      RGBA32U,
      RGBA32F,
      B5G6R5,
      R5G6B5,
      BGRA4,
      RGBA4,
      BGR5A1,
      RGB5A1,
      RGB10A2,
      RG11B10F,

      DEPTHF,

      /// add more here...

      NUM_FORMATS
    };

    enum filter : uint32_t {
      NEAREST = 0,
      LINEAR,
      NEAREST_MIPMAP_NEAREST,
      LINEAR_MIPMAP_NEAREST,
      NEAREST_MIPMAP_LINEAR,
      LINEAR_MIPMAP_LINEAR,

      /// add more here...

      NUM_FILTERS
    };

    enum wrap : uint32_t {
      CLAMP_TO_EDGE = 0,
      MIRRORED_REPEAT,
      REPEAT,
      CLAMP_TO_BORDER,

      /// add more here...

      NUM_WRAP_MODES
    };

    texture() = default;
    texture(resource_handle handle)
        : resource(handle) {}
    virtual ~texture() = default;

    resource_type type() const override { return resource_type::TEXTURE; }

    static resource_handle create(const std::string& name, tex_type type = TEXTURE_2D, format frmt = format::RGBA8, uint32_t width = 0, uint32_t height = 0);
    static resource_handle create(
      const std::string& name, tex_type type = TEXTURE_2D, format frmt = format::RGBA8,
      const std::pair<filter, filter>& filters = { LINEAR, LINEAR },
      const std::tuple<wrap, wrap, wrap>& wraps = { CLAMP_TO_EDGE, CLAMP_TO_EDGE, CLAMP_TO_EDGE },
      uint32_t width = 0, uint32_t height = 0
    );
    static resource_handle create3d(const std::string& name, format frmt = format::RGBA8, const glm::vec3& dimensions = glm::vec3(0));

    texture& bind(uint32_t slot = 0);
    texture& bind_image(uint32_t index, uint32_t level, bool layered, int32_t layer = 0, format frmt = format::RGBA32F, access_flags flags = access_flags::READ_WRITE);

    texture& set_type(tex_type type);
    texture& set_size(uint32_t width, uint32_t height);
    texture& set_format(format frmt);
    texture& set_filter(filter min_filter, filter mag_filter = NEAREST);
    texture& set_wrap_mode(wrap wrap_s, wrap wrap_t = CLAMP_TO_EDGE, wrap wrap_r = CLAMP_TO_EDGE);
    texture& set_data(void* data, size_t size);

    void unbind(uint32_t slot = 0);
    void finalize_texture();

    ImTextureID get_imgui_texture_id();

    const void* get_data() const { return data; }
    tex_type get_type() const { return (tex_type)texture_type; }
    const glm::ivec2& get_size() const { return size; }
    format get_format() const { return (format)texture_format; }
    filter get_min_filter() const { return (filter)min_filter; }
    filter get_mag_filter() const { return (filter)mag_filter; }
    wrap get_wrap_s() const { return (wrap)wrap_s; }
    wrap get_wrap_t() const { return (wrap)wrap_t; }
    wrap get_wrap_r() const { return (wrap)wrap_r; }

    tex_type texture_type = tex_type::TEXTURE_2D;

    glm::ivec2 size = { 0, 0 };
    void* data = nullptr;
    size_t data_size = 0;

    format texture_format = format::RGBA8;

    filter min_filter = filter::LINEAR;
    filter mag_filter = filter::LINEAR;

    wrap wrap_s = wrap::CLAMP_TO_EDGE;
    wrap wrap_t = wrap::CLAMP_TO_EDGE;
    wrap wrap_r = wrap::CLAMP_TO_EDGE;
  };

}  // namespace other

OTHER_REFLECT(
  other::texture
  // ,
  // field(texture_type, other::attr::serializable()),
  // field(size, other::attr::serializable()),
  // field(texture_format, other::attr::serializable()),
  // field(min_filter, other::attr::serializable()),
  // field(mag_filter, other::attr::serializable()),
  // field(wrap_s, other::attr::serializable()),
  // field(wrap_t, other::attr::serializable()),
  // field(wrap_r, other::attr::serializable())
);

#endif  // OTHER_RENDERER_GPU_RESOURCE_TEXTURE_HPP