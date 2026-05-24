/**
 * \file renderer/render_pass.cpp
 **/
#include "renderer/render_pass.hpp"

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/shader.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void render_pass::bind_pass(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

    if (shader_handle.has_value()) {
      if (framebuffer_handle.has_value()) {
        renderer_ptr->get_resource<framebuffer>(*framebuffer_handle).bind();
      }
      renderer_ptr->get_resource<shader>(*shader_handle).bind();
    }
  }

  void render_pass::unbind_pass(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

    if (shader_handle.has_value()) {
      if (framebuffer_handle.has_value()) {
        renderer_ptr->get_resource<framebuffer>(*framebuffer_handle).unbind();
      }
      renderer_ptr->get_resource<shader>(*shader_handle).unbind();
    }
  }

}  // namespace other