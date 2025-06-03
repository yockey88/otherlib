/**
 * \file gpu_resource/framebuffer.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_FRAMEBUFFER_HPP
#define OTHER_RENDERER_GPU_RESOURCE_FRAMEBUFFER_HPP

#include <glm/glm.hpp>

#include "serialization/reflection.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/texture.hpp"

#include "glm/fwd.hpp"

namespace other {

  struct framebuffer : public resource {
    enum attachment_type {
      COLOR = 0,
      DEPTH,
      STENCIL,
      DEPTH_STENCIL,

      /// add more here...

      NUM_ATTACHMENT_TYPES,
      NO_ATTACHMENTS = NUM_ATTACHMENT_TYPES
    };
    OTHER_REFLECTABLE(framebuffer);

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::ivec2 size = { 0, 0 };

    framebuffer() = default;
    framebuffer(resource_handle handle)
        : resource(handle) {}
    virtual ~framebuffer() = default;

    resource_type type() const override { return resource_type::FRAMEBUFFER; }

    framebuffer& bind();
    framebuffer& set_size(uint32_t width, uint32_t height);
    framebuffer& set_clear_color(const glm::vec4& color);

    // texture& set_type(tex_type type);
    // texture& set_size(uint32_t width, uint32_t height);
    // texture& set_format(format frmt);
    // texture& set_filter(filter min_filter, filter mag_filter = NEAREST);
    // texture& set_wrap_mode(wrap wrap_s, wrap wrap_t = CLAMP_TO_EDGE, wrap wrap_r = CLAMP_TO_EDGE);
    // texture& set_data(const std::vector<uint8_t>& data);
    // texture& set_data(const uint8_t* data, size_t size);

    // clang-format off
    framebuffer& add_attachment(const std::string& text_name, attachment_type type, texture::tex_type tex_type = texture::tex_type::TEXTURE_2D, texture::format format = texture::format::RGBA32F, 
                                texture::filter min_filter = texture::filter::LINEAR, texture::filter max_filter = texture::filter::LINEAR, 
                                texture::wrap wrap_s = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap = texture::wrap::CLAMP_TO_EDGE);
    // clang-format on
    framebuffer& add_attachment(const resource_handle& attachment_handle, attachment_type type);

    void unbind();
    void finalize_framebuffer();
    void destroy_resources();

    // COLOR, DEPTH, STENCIL, DEPTH_STENCIL
    std::array<opt<resource_handle>, 4> attachment_textures;

    bool complete = false;
    bool ready_to_finalize = false;

   private:
    attachment_type final_type = attachment_type::COLOR;

    opt<std::string> error_msg;

    void check_build_status();
  };

}  // namespace other

OTHER_REFLECT(
  other::framebuffer
)

#endif  // OTHER_RENDERER_GPU_RESOURCE_FRAMEBUFFER_HPP
