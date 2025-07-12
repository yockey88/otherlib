/**
 * \file renderer/render_pipeline.cpp
 **/
#include "renderer/render_pipeline.hpp"

#include <algorithm>
#include <string>

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void render_pipeline::initialize_pipeline(renderer* renderer) {
    graph = arena_allocator<render_graph>{}.allocate(renderer);
    create_resources();

    graph->start_pipeline();
    build_render_passes();
    graph->end_pipeline();
  }

  void render_pipeline::shutdown_pipeline() {
    destroy_resources();

    arena_allocator<render_graph>{}.free(graph);
    graph = nullptr;
  }

  void render_pipeline::begin_frame(render_data* data) {
    OTHER_ASSERT(graph != nullptr, "Render graph is not initialized. Cannot begin frame.");
    OTHER_ASSERT(data != nullptr, "Render data must not be null.");
    if (!valid) {
      CORE_LOG_ERROR("Render pipeline is not valid, cannot begin frame.");
      return;
    }

    graph->get_renderer()->submit_render_data(data);
    frame_resources = {
      .model_buffer = model_buffer_handle.value(),
      .material_buffer = material_buffer_handle.value(),
    };
    graph->get_renderer()->begin_frame(&frame_resources);
  }

  void render_pipeline::execute_frame() {
    graph->get_renderer()->render(*graph);
  }

  void render_pipeline::end_frame() {
    graph->get_renderer()->end_frame();
  }

  void render_pipeline::set_material_buffer(const std::string_view name) {
    auto itr = std::ranges::find_if(buffer_resources, [&](const auto& pair) {
      return pair.second.name == name;
    });
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Material buffer resource [{}] not found in pipeline.", name);
      return;
    }
    material_buffer_handle = itr->second.handle;
  }

  void render_pipeline::set_model_buffer(const std::string_view name) {
    auto itr = std::ranges::find_if(buffer_resources, [&](const auto& pair) {
      return pair.second.name == name;
    });
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Model buffer resource [{}] not found in pipeline.", name);
      return;
    }
    model_buffer_handle = itr->second.handle;
  }

  void render_pipeline::add_buffer_resource(const std::string_view name, gpu_buffer::buf_type type, gpu_buffer::usage usage) {
    resource_handle handle = gpu_buffer::create(name, type, usage);
    auto [itr, inserted] = buffer_resources.insert({ handle.id, { .name = std::string{ name }, .handle = handle } });
    OTHER_ASSERT(inserted, "Failed to insert buffer resource with name [{}].", name);
  }

  void render_pipeline::add_texture_resource(const std::string_view name, const glm::vec2& size, framebuffer::attachment_type type) {
    resource_handle handle = texture::create(std::string{ name }, texture::tex_type::TEXTURE_2D, texture::format::RGBA32F, size.x, size.y);
    auto [itr, inserted] = texture_resources.insert({ handle.id, { .name = std::string{ name }, .handle = handle } });
    OTHER_ASSERT(inserted, "Failed to insert texture resource with name [{}].", name);
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::buffer_resource(const std::string_view name, access_flags flags) {
    auto handle = pipeline->find_buffer_resource(name);
    if (!handle.has_value()) {
      CORE_LOG_ERROR("Buffer resource [{}] not found in pipeline.", name);
      return *this;
    }

    builder.buffer_resource(*handle, curr_buffer_binding++, flags);
    return *this;
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::texture_resource(const std::string_view name, framebuffer::attachment_type type, access_flags flags) {
    auto handle = pipeline->find_texture_resource(name);
    if (!handle.has_value()) {
      CORE_LOG_ERROR("Texture resource [{}] not found in pipeline.", name);
      return *this;
    }

    builder.texture_resource(*handle, curr_texture_slot++, type, flags);
    return *this;
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::execution_callback(render_graph::pass_executor&& executor, void* user_data) {
    builder.execution_callback(std::move(executor), user_data);
    return *this;
  }

  void render_pipeline::pass_builder::end_pass() {
    (void)builder.end_pass();
  }

  render_pipeline::pass_builder render_pipeline::start_pass(const std::string_view name, resource_handle shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer) {
    OTHER_ASSERT(graph != nullptr, "Render graph is not initialized. Cannot start a new pass.");
    return { this, graph->start_pass(name, shader_handle, rptype, size, create_framebuffer) };
  }

  opt<resource_handle> render_pipeline::find_buffer_resource(const std::string_view name) const {
    auto itr = std::ranges::find_if(buffer_resources, [&](const auto& pair) { return pair.second.name == name; });
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource [{}] not found in pipeline.", name);
      return std::nullopt;
    }
    return itr->second.handle;
  }

  opt<resource_handle> render_pipeline::find_texture_resource(const std::string_view name) const {
    auto itr = std::ranges::find_if(texture_resources, [&](const auto& pair) { return pair.second.name == name; });
    if (itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource [{}] not found in pipeline.", name);
      return std::nullopt;
    }
    return itr->second.handle;
  }

}  // namespace other