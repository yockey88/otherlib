/**
 * \file gpu_resource/framebuffer.cpp
 **/
#include "gpu_resource/framebuffer.hpp"

#include "renderer/renderer_backend.hpp"

namespace other {

  framebuffer& framebuffer::bind() {
    subsystem<renderer_backend>::get()->api()->bind_framebuffer_resource(handle());
    return *this;
  }

  framebuffer& framebuffer::set_clear_color(const glm::vec4& color) {
    clear_color = color;
    return *this;
  }

  framebuffer& framebuffer::set_size(uint32_t width, uint32_t height) {
    size = { width, height };
    return *this;
  }

  // clang-format off
  framebuffer& framebuffer::add_attachment(const std::string& text_name, attachment_type type, texture::tex_type tex_type, texture::format format, 
                                            texture::filter min_filter, texture::filter max_filter, 
                                            texture::wrap wrap_s, texture::wrap wrap_t, texture::wrap wrap) {
    // clang-format on
    OTHER_ASSERT(type < attachment_type::NUM_ATTACHMENT_TYPES, "Invalid attachment type: {}", type);
    if (size.x == 0 || size.y == 0) {
      CORE_LOG_ERROR("Framebuffer size must be set before adding attachments.");
      return *this;
    }

    resource_handle attachment = subsystem<renderer_backend>::get()->api()->create_resource(text_name + std::format("_{}_", type) + "_attach", resource_type::TEXTURE);
    (*subsystem<renderer_backend>::get()->api()->get_resource_as<texture>(attachment))
      .set_type(tex_type)
      .set_format(format)
      .set_size(size.x, size.y)
      .set_filter(texture::filter::LINEAR, texture::filter::LINEAR)
      .set_wrap_mode(texture::wrap::CLAMP_TO_EDGE, texture::wrap::CLAMP_TO_EDGE)
      .finalize_texture();

    return add_attachment(attachment, type);
  }

  framebuffer& framebuffer::add_attachment(const resource_handle& attachment_handle, attachment_type type) {
    if (attachment_handle.id == 0 || attachment_handle.type != resource_type::TEXTURE) {
      CORE_LOG_ERROR("Invalid attachment handle: must be a valid texture resource.");
      return *this;
    }

    if (type == attachment_type::COLOR) {
      uint32_t color_idx = color_attachments.size();
      resource_handle& attachment = color_attachments.emplace_back();
      attachment = attachment_handle;
      subsystem<renderer_backend>::get()->api()->framebuffer_texture_2d(handle(), attachment, type, 0, color_idx);
    } else {
      opt<resource_handle>& attachment = attachment_textures[static_cast<size_t>(type)];
      if (attachment.has_value()) {
        CORE_LOG_ERROR("Attachment of type COLOR already exists in framebuffer, cannot add again.");
        return *this;
      }

      attachment = attachment_handle;
      subsystem<renderer_backend>::get()->api()->framebuffer_texture_2d(handle(), *attachment, type, 0);
    }

    return *this;
  }

  void framebuffer::unbind() {
    subsystem<renderer_backend>::get()->api()->unbind_framebuffer_resource(handle());
  }

  void framebuffer::finalize_framebuffer() {
    check_build_status();
    if (!ready_to_finalize) {
      CORE_LOG_ERROR("Framebuffer is not complete, cannot finalize.");
      return;
    }

    if (size.x == 0 || size.y == 0) {
      CORE_LOG_ERROR("Framebuffer size is not set, cannot finalize.");
      return;
    }

    subsystem<renderer_backend>::get()->api()->finalize_framebuffer(handle());
  }

  void framebuffer::destroy_resources() {
    for (auto& attachment : attachment_textures) {
      if (attachment.has_value()) {
        subsystem<renderer_backend>::get()->api()->destroy_resource(*attachment);
        attachment = std::nullopt;
      }
    }
    complete = false;
    final_type = attachment_type::NO_ATTACHMENTS;
  }

  void framebuffer::check_build_status() {
    enum attachment_flags {
      COLOR_ATTACHMENT = 1 << 0,
      DEPTH_ATTACHMENT = 1 << 1,
      STENCIL_ATTACHMENT = 1 << 2,
      DEPTH_STENCIL_ATTACHMENT = 1 << 3,
    };

    uint8_t flags = 0;
    if (color_attachments.size() > 0) {
      flags |= COLOR_ATTACHMENT;
    }
    if (attachment_textures[static_cast<size_t>(attachment_type::DEPTH)].has_value()) {
      flags |= DEPTH_ATTACHMENT;
    }
    if (attachment_textures[static_cast<size_t>(attachment_type::STENCIL)].has_value()) {
      flags |= STENCIL_ATTACHMENT;
    }
    if (attachment_textures[static_cast<size_t>(attachment_type::DEPTH_STENCIL)].has_value()) {
      flags |= DEPTH_STENCIL_ATTACHMENT;
    }

    if ((flags & DEPTH_ATTACHMENT) && (flags & STENCIL_ATTACHMENT)) {
      final_type = attachment_type::DEPTH_STENCIL;
    } else if ((flags & COLOR_ATTACHMENT)) {
      final_type = attachment_type::COLOR;
    } else if ((flags & DEPTH_ATTACHMENT)) {
      final_type = attachment_type::DEPTH;
    } else if ((flags & STENCIL_ATTACHMENT)) {
      final_type = attachment_type::STENCIL;
    } else {
      CORE_LOG_ERROR("Framebuffer has no valid attachments: {}", error_msg.has_value() ? *error_msg : "No attachments added.");
      final_type = attachment_type::NO_ATTACHMENTS;
    }

    if (final_type != attachment_type::NO_ATTACHMENTS) {
      ready_to_finalize = true;
    }
  }

}  // namespace other