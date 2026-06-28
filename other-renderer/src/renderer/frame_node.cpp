/**
 * \file renderer/frame_node.cpp
 **/
#include "renderer/frame_node.hpp"

#include "gpu_resource/gpu_buffer.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void frame_node::start_pass(renderer* renderer_ptr, pass_runtime* runtime) const {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");
    OTHER_ASSERT(runtime != nullptr, "Pass runtime pointer must not be null.");

    pass_begin_info info{
      .framebuffer = pass->framebuffer_handle,
      .pass_name = pass->name,
      .render_area_size = pass->size,
      .clear_color = pass->clear_color,  // see note
      .clear_depth = std::nullopt,
      .pass_type = pass->pass_type,
    };
    renderer_ptr->rendering()->api()->begin_pass(info);

    if (pass->shader_handle.has_value()) {
      pass->bind_pass(renderer_ptr);
      auto& sh = renderer_ptr->get_resource<shader>(*pass->shader_handle);

      for (const auto& [id, buffer] : input_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(buffer.binding_point, *pass->shader_handle)
          .bind_to_shader()
          .bind();
      }
      for (const auto& [id, buffer] : output_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(buffer.binding_point, *pass->shader_handle)
          .bind_to_shader()
          .bind();
      }

      if (pass->pass_type == render_pass::RENDER_PASS) {
        for (const auto& [id, tex] : input_textures) {
          renderer_ptr->get_resource<texture>(tex.handle).bind(tex.slot);
        }
      } else if (pass->pass_type == render_pass::COMPUTE_PASS) {
        for (const auto& [id, tex] : input_textures) {
          auto& t = renderer_ptr->get_resource<texture>(tex.handle);
          if (tex.flags & SAMPLE) {
            t.bind(tex.slot);
          } else {
            t.bind_image(tex.slot, tex.mip_level, true, 0, t.get_format(), READ);
          }
        }
        for (const auto& [id, tex] : output_textures) {
          auto& t = renderer_ptr->get_resource<texture>(tex.handle);
          t.bind_image(tex.slot, tex.mip_level, true, 0, t.get_format(), WRITE);
        }
      }

      for (const auto& [id, tex] : input_textures) {
        if (tex.uniform_name.empty()) {
          continue;
        }
        sh.set_uniform(tex.uniform_name, int(tex.slot));
      }
      for (const auto& [id, tex] : output_textures) {
        if (tex.uniform_name.empty()) {
          continue;
        }
        sh.set_uniform(tex.uniform_name, int(tex.slot));
      }

      for (const auto& [id, uniform] : pass->uniforms) {
        if (uniform.name.empty() || uniform.val.is_empty()) {
          continue;
        }
        switch (uniform.val.type()) {
          case value_type::FLOAT: sh.set_uniform(uniform.name, (float)uniform.val); break;
          case value_type::VEC2: {
            glm::vec2 vec2_val = uniform.val;
            sh.set_uniform(uniform.name, vec2_val);
          } break;
          case value_type::VEC3: {
            glm::vec3 vec3_val = uniform.val;
            sh.set_uniform(uniform.name, vec3_val);
          } break;
          case value_type::VEC4: {
            glm::vec4 vec4_val = uniform.val;
            sh.set_uniform(uniform.name, vec4_val);
          } break;
          case value_type::INT8:
          case value_type::INT16:
          case value_type::INT32:
            sh.set_uniform(uniform.name, (int32_t)uniform.val);
            break;
          case value_type::UINT8:
          case value_type::UINT16:
          case value_type::UINT32:
            sh.set_uniform(uniform.name, (uint32_t)uniform.val);
            break;
          default:
            CORE_LOG_ERROR("Unsupported uniform type for uniform '{}': {}", uniform.name, uniform.val.type());
        }
      }
    }
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

      for (const auto& [id, buffer] : output_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      for (const auto& [id, buffer] : input_buffers) {
        renderer_ptr->get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      pass->unbind_pass(renderer_ptr);
    }

    renderer_ptr->rendering()->api()->end_pass();
  }

}  // namespace other