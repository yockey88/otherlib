/**
 * \file renderer/render_graph.cpp
 **/
#include "renderer/render_graph.hpp"

namespace other {

  render_graph::pass_builder& render_graph::pass_builder::add_color_attachment(resource_handle texture_id, uint32_t binding) {
    pass.color_attachments.push_back(texture_id);
    pass.color_attachment_binding_points.push_back(binding);
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::use_buffer_resource(resource_handle texture_id, uint32_t binding) {
    pass.buffer_resources.push_back(texture_id);
    pass.buffer_resource_binding_points.push_back(binding);
    return *this;
  }

  render_graph& render_graph::pass_builder::end_pass() {
    OTHER_ASSERT(pass.graph != nullptr, "Render pass graph is null.");
    if (pass.size.x == 0 || pass.size.y == 0) {
      CORE_LOG_ERROR("Render pass size is zero, using swapchain size instead.");
      pass.size = { 800, 600 };  // Default size, should be replaced with actual swapchain size
    }

    if (pass.shader_handle.id == 0) {
      CORE_LOG_ERROR("Render pass shader handle is invalid, cannot finalize pass.");
    } else {
    }

    return *pass.graph;
  }

  render_graph::pass_builder render_graph::start_pass(natural_t id, const glm::ivec2& size, resource_handle shader_handle) {
    if (current_pass != nullptr) {
      /// add this pass as a child of the current pass
    }
    /// this will be the root pass
    else {
    }

    if (auto itr = passes.find(id); itr != passes.end()) {
      CORE_LOG_ERROR("Render pass with ID {} already exists.", id);
      if (current_pass == &itr->second) {
        CORE_LOG_ERROR("Cannot create a pass with the same ID as the current pass.");
        return pass_builder(itr->second);
      }
      OTHER_ASSERT(false, "Render pass with the same ID already exists.");
    }

    auto& pass = passes[id] = render_pass(this, id);

    pass.id = id;
    pass.graph = this;

    pass.size = size;

    pass.shader_handle = shader_handle;
    pass.user_data = nullptr;

    current_pass = &pass;
    return pass;
  }

  render_graph& render_graph::add_buffer_resource(resource_handle buffer_id, uint32_t binding) {
    for (auto& pass : passes) {
      pass.second.buffer_resources.push_back(buffer_id);
      pass.second.buffer_resource_binding_points.push_back(binding);
    }
    return *this;
  }

}  // namespace other