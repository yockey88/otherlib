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
      DEPTH = 0,
      STENCIL,
      DEPTH_STENCIL,

      COLOR,

      /// add more here...

      NUM_ATTACHMENT_TYPES,
      NO_ATTACHMENTS = NUM_ATTACHMENT_TYPES
    };
    enum clear_mask_bit {
      NONE = 0,
      COLOR_BIT = 1 << 0,
      DEPTH_BIT = 1 << 1,
      STENCIL_BIT = 1 << 2,
      ALL_BITS = COLOR_BIT | DEPTH_BIT | STENCIL_BIT
    };

    glm::vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
    glm::ivec2 size = { 0, 0 };
    uint32_t samples = 1;

    framebuffer() = default;
    framebuffer(resource_handle handle)
        : resource(handle) {}
    virtual ~framebuffer() = default;

    resource_type type() const override { return resource_type::FRAMEBUFFER; }

    framebuffer& bind();
    framebuffer& set_samples(uint32_t samples);
    framebuffer& set_size(uint32_t width, uint32_t height);
    framebuffer& set_clear_color(const glm::vec4& color);

    // clang-format off
    framebuffer& add_attachment(const std::string& text_name, attachment_type type, texture::tex_type tex_type = texture::tex_type::TEXTURE_2D, texture::format format = texture::format::RGBA32F, 
                                texture::filter min_filter = texture::filter::LINEAR, texture::filter max_filter = texture::filter::LINEAR, 
                                texture::wrap wrap_s = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap_t = texture::wrap::CLAMP_TO_EDGE, texture::wrap wrap = texture::wrap::CLAMP_TO_EDGE);
    // clang-format on
    framebuffer& add_attachment(const resource_handle& attachment_handle, attachment_type type, uint32_t mip_level = 0);

    void unbind();
    void finalize_framebuffer();

    std::vector<resource_handle> color_attachments;
    // DEPTH, STENCIL, DEPTH_STENCIL
    std::array<opt<resource_handle>, 3> attachment_textures;

    bool complete = false;
    bool ready_to_finalize = false;

    int32_t clear_mask = clear_mask_bit::NONE;

   private:
    attachment_type final_type = attachment_type::COLOR;
    opt<std::string> error_msg;

    void check_build_status();
  };

}  // namespace other

OTHER_REFLECT(
  other::framebuffer)

#endif  // OTHER_RENDERER_GPU_RESOURCE_FRAMEBUFFER_HPP
