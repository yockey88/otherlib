/**
 * \file renderer/frame_node.cpp
 **/
#include "renderer/frame_node.hpp"

#include "gpu_resource/gpu_buffer.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void frame_node::start_pass(renderer* renderer_ptr) const {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

    pass_begin_info info{
      .framebuffer = pass->framebuffer_handle,
      .render_area_size = pass->size,
      .clear_color = pass->clear_color,  // see note
      .clear_depth = std::nullopt,
      .pass_type = pass->pass_type,
    };
    renderer_ptr->rendering()->api()->begin_pass(info);

    if (pass->shader_handle.has_value()) {
      pass->bind_pass(renderer_ptr);
      for (const auto& [binding_point, buffer] : input_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(binding_point, *pass->shader_handle)
          .bind();
      }
      for (const auto& [binding_point, buffer] : output_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(binding_point, *pass->shader_handle)
          .bind();
      }

      if (pass->pass_type == render_pass::RENDER_PASS) {
        for (const auto& [id, tex] : input_textures) {
          renderer_ptr->get_resource<texture>(tex.handle).bind(tex.slot);
        }
      } else if (pass->pass_type == render_pass::COMPUTE_PASS) {
        for (const auto& [id, tex] : input_textures) {
          auto& t = renderer_ptr->get_resource<texture>(tex.handle);
          t.bind_image(tex.slot, 0, true, 0, t.get_format(), READ);
        }
        for (const auto& [id, tex] : output_textures) {
          auto& t = renderer_ptr->get_resource<texture>(tex.handle);
          t.bind_image(tex.slot, 0, true, 0, t.get_format(), WRITE);
        }
      }
    }
    /// set other pipeline state options here
  }

  void frame_node::end_pass(renderer* renderer_ptr) const {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

    if (pass->shader_handle.has_value()) {
      for (const auto& [id, tex] : input_textures) {
        renderer_ptr->get_resource<texture>(tex.handle).unbind(tex.slot);
      }
      if (pass->pass_type == render_pass::COMPUTE_PASS) {
        for (const auto& [id, tex] : output_textures) {
          renderer_ptr->get_resource<texture>(tex.handle).unbind(tex.slot);
        }
      }

      for (const auto& [binding_point, buffer] : output_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      for (const auto& [binding_point, buffer] : input_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      pass->unbind_pass(renderer_ptr);
    }

    renderer_ptr->rendering()->api()->end_pass();
  }

}  // namespace other