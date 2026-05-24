/**
 * \file renderer/frame_node.cpp
 **/
#include "renderer/frame_node.hpp"

#include "gpu_resource/gpu_buffer.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void frame_node::start_pass(renderer* renderer_ptr) const {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");

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

      for (const auto& [id, tex] : input_textures) {
        renderer_ptr->get_resource<texture>(tex.handle).bind(tex.slot);
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

      for (const auto& [binding_point, buffer] : output_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      for (const auto& [binding_point, buffer] : input_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      pass->unbind_pass(renderer_ptr);
    }
  }

}  // namespace other