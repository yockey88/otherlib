/**
 * \file renderer/render_pass.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RENDER_PASS_HPP
#define OTHER_RENDERER_RENDERER_RENDER_PASS_HPP

#include "core/value.hpp"

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/renderer_resource.hpp"

namespace other {

  class renderer;
  class render_graph;

  struct render_pass {
    enum type {
      RENDER_PASS = 0,
      COMPUTE_PASS,
    } pass_type = RENDER_PASS;
    framebuffer::clear_mask_bit clear_flags = framebuffer::ALL_BITS;
    bool override_fb_clear = false;

    natural_t id = 0;
    opt<resource_handle> framebuffer_handle = std::nullopt;
    opt<resource_handle> shader_handle = {};
    void* user_data = nullptr;

    /// for other dynamic resource binding later
    natural_t next_texture_id = 0;
    natural_t next_buffer_id = 0;

    std::string name;
    glm::ivec2 size = { 0, 0 };
    uint32_t samples = 1;
    glm::vec4 clear_color = { 0.2, 0.2, 0.2, 1.0 };

    struct texture_resource {
      std::string uniform_name;  //< for samplerXD uniforms, imageXD uniforms, or bindless resource indexing

      framebuffer::attachment_type type;
      natural_t slot;
      access_flags flags;
      resource_handle handle;
      uint32_t mip_level = 0;
    };
    struct buffer_resource {
      natural_t binding_point;
      access_flags flags;
      resource_handle handle;
    };
    struct uniform {
      std::string name;
      value val;
    };

    std::vector<std::string> depends_on;
    std::map<natural_t, texture_resource> texture_resources;
    std::map<natural_t, buffer_resource> buffer_resources;
    std::map<natural_t, uniform> uniforms;

    void bind_pass(renderer* renderer_ptr);
    void unbind_pass(renderer* renderer_ptr);
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDER_PASS_HPP