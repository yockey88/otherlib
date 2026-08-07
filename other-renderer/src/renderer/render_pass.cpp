/**
 * \file renderer/render_pass.cpp
 **/
#include "renderer/render_pass.hpp"

#include "core/profiler.hpp"

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/shader.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void render_pass::bind_pass(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");
    PROFILE_SECTION("render_pass::bind_pass");

    if (framebuffer_handle.has_value()) {
      renderer_ptr->get_resource<framebuffer>(*framebuffer_handle).bind(!override_fb_clear);
    }

    if (pass_type != render_pass::COMPUTE_PASS && (!framebuffer_handle.has_value() || override_fb_clear)) {
      renderer_ptr->rendering()->api()->set_viewport(0, 0, renderer_ptr->get_window_size().x, renderer_ptr->get_window_size().y);
      renderer_ptr->rendering()->api()->clear_viewport(clear_color, clear_flags);
    }

    if (shader_handle.has_value()) {
      renderer_ptr->get_resource<shader>(*shader_handle).bind();
    }
  }

  void render_pass::unbind_pass(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

    if (shader_handle.has_value()) {
      renderer_ptr->get_resource<shader>(*shader_handle).unbind();
    }

    if (framebuffer_handle.has_value()) {
      renderer_ptr->get_resource<framebuffer>(*framebuffer_handle).unbind();
    }
  }

}  // namespace other