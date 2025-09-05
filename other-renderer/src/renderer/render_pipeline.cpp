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

    validate_pipeline();
  }

  void render_pipeline::shutdown_pipeline() {
    destroy_resources();

    arena_allocator<render_graph>{}.free(graph);
    graph = nullptr;
  }

  void render_pipeline::upload_buffer(const std::string_view name, const void* data, size_t size) {
    gpu_buffer* buffer = get_resource<gpu_buffer>(name);
    if (buffer == nullptr) {
      CORE_LOG_ERROR("Buffer resource [{}] not found in pipeline.", name);
      return;
    }
    buffer
      ->set_data(data, size)
      .finalize_buffer();
  }

  void render_pipeline::prepare_frame(renderer::frame_resources* resources, render_data* data) {
    OTHER_ASSERT(resources != nullptr, "Frame resources must not be null.");
    PROFILE_SECTION("render_pipeline::prepare_frame");

    frame_render_data = data;
    on_prepare_frame(resources, data);
  }

  void render_pipeline::render_frame(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");
    PROFILE_SECTION("render_pipeline::render_frame");

    const auto& g = graph->get_graph();
    const auto& execs = graph->get_executors();
    for (const natural_t id : graph->get_topological_sort()) {
      auto node_atr = g.nodes.find(id);
      OTHER_ASSERT(node_atr != g.nodes.end(), "Node with id {} not found in graph.", id);

      const auto& n = node_atr->second;
      const auto* pass = n.pass;
      auto itr = execs.find(pass->id);
      OTHER_ASSERT(itr != execs.end(), "Executor for pass {} not found.", id);

      n.start_pass(renderer_ptr);
      itr->second.operator()(*renderer_ptr, &n, pass->user_data);
      n.end_pass(renderer_ptr);
    }
  }

  renderer::frame_resources render_pipeline::get_frame_resources() const {
    return {
      .model_buffer = *model_buffer_handle,
      .material_buffer = *material_buffer_handle,
      .point_light_buffer = *point_light_buffer_handle,
      .direction_light_buffer = *direction_light_buffer_handle,
      .camera_buffer = *camera_buffer_handle
    };
  }

  void render_pipeline::destroy_resources() {
    for (const auto& [id, resource] : buffer_resources) {
      get_renderer()->destroy_resource(resource.handle);
    }
    for (const auto& [id, resource] : texture_resources) {
      get_renderer()->destroy_resource(resource.handle);
    }
    buffer_resources.clear();
    texture_resources.clear();

    model_buffer_handle = std::nullopt;
    material_buffer_handle = std::nullopt;
    point_light_buffer_handle = std::nullopt;
    direction_light_buffer_handle = std::nullopt;
    camera_buffer_handle = std::nullopt;
  }

  void render_pipeline::set_material_buffer(const std::string_view name) {
    set_core_buffer(material_buffer_handle, name);
  }

  void render_pipeline::set_model_buffer(const std::string_view name) {
    set_core_buffer(model_buffer_handle, name);
  }

  void render_pipeline::set_point_light_buffer(const std::string_view name) {
    set_core_buffer(point_light_buffer_handle, name);
  }

  void render_pipeline::set_direction_light_buffer(const std::string_view name) {
    set_core_buffer(direction_light_buffer_handle, name);
  }

  void render_pipeline::set_camera_buffer(const std::string_view name) {
    set_core_buffer(camera_buffer_handle, name);
  }

  void render_pipeline::add_buffer_resource(const std::string_view name, gpu_buffer::buf_type type, gpu_buffer::usage usage) {
    resource_handle handle = gpu_buffer::create(name, type, usage);
    auto [itr, inserted] = buffer_resources.insert({ handle.id, { .name = std::string{ name }, .handle = handle } });
    OTHER_ASSERT(inserted, "Failed to insert buffer resource with name [{}].", name);
  }

  void render_pipeline::add_texture_resource(const std::string_view name, const glm::vec2& size, texture::tex_type tex_type, texture::format format) {
    resource_handle handle;
    if (tex_type == texture::tex_type::TEXTURE_CUBE) {
      handle = cube_map::create(std::string{ name }, format, size.x, size.y);
    } else {
      handle = texture::create(std::string{ name }, tex_type, format, size.x, size.y);
    }
    auto [itr, inserted] = texture_resources.insert({ handle.id, { .name = std::string{ name }, .handle = handle } });
    OTHER_ASSERT(inserted, "Failed to insert texture resource with name [{}].", name);
  }

  void render_pipeline::add_texture_resource(const std::string_view name, const glm::vec2& size, texture::tex_type tex_type, texture::format format, const std::pair<texture::filter, texture::filter>& filters, const std::tuple<texture::wrap, texture::wrap, texture::wrap>& wraps) {
    resource_handle handle = texture::create(std::string{ name }, tex_type, format, filters, wraps, size.x, size.y);
    auto [itr, inserted] = texture_resources.insert({ handle.id, { .name = std::string{ name }, .handle = handle } });
    OTHER_ASSERT(inserted, "Failed to insert texture resource with name [{}].", name);
  }

  shader* render_pipeline::get_pass_shader(const std::string_view name) {
    auto itr = std::ranges::find_if(graph->get_graph().nodes, [&](const auto& pair) { return pair.second.pass->name == name; });
    if (itr == graph->get_graph().nodes.end()) {
      CORE_LOG_ERROR("Shader pass [{}] not found in graph.", name);
      return nullptr;
    }

    if (!itr->second.pass->shader_handle.has_value()) {
      return nullptr;
    }
    return &get_renderer()->get_resource<shader>(*itr->second.pass->shader_handle);
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::clear_color(const glm::vec4& clear_color) {
    builder.set_clear_color(clear_color);
    return *this;
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::buffer_resource(const std::string_view name, uint32_t binding, access_flags flags) {
    auto handle = pipeline->find_buffer_resource(name);
    if (!handle.has_value()) {
      CORE_LOG_ERROR("Buffer resource [{}] not found in pipeline.", name);
      return *this;
    }

    CORE_LOG_DEBUG("Adding buffer resource [{}]", name);
    builder.buffer_resource(*handle, binding, flags);
    return *this;
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::texture_resource(const std::string_view name, framebuffer::attachment_type type, access_flags flags) {
    auto handle = pipeline->find_texture_resource(name);
    if (!handle.has_value()) {
      CORE_LOG_ERROR("Texture resource [{}] not found in pipeline.", name);
      return *this;
    }

    uint32_t slot = 0;
    if ((flags & WRITE) != WRITE) {
      slot = curr_texture_slot++;
      CORE_LOG_DEBUG("Adding texture resource [{}] with slot [{}]", name, slot);
    } else {
      CORE_LOG_DEBUG("Adding texture resource [{}] pass framebuffer", name);
    }

    CORE_LOG_DEBUG("Adding texture resource [{}] with slot [{}]", name, slot);
    builder.texture_resource(*handle, slot, type, flags);
    return *this;
  }

  render_pipeline::pass_builder& render_pipeline::pass_builder::execution_callback(render_graph::pass_executor&& executor, void* user_data) {
    builder.execution_callback(std::move(executor), user_data);
    return *this;
  }

  void render_pipeline::pass_builder::end_pass() {
    CORE_LOG_DEBUG("Finalizing pass [{}]", pass_name);
    (void)builder.end_pass();
  }

  render_pipeline::pass_builder render_pipeline::start_pass(const std::string_view name, opt<resource_handle> shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer) {
    OTHER_ASSERT(graph != nullptr, "Render graph is not initialized. Cannot start a new pass.");
    auto builder = render_pipeline::pass_builder{ this, graph->start_pass(name, shader_handle, rptype, size, create_framebuffer) };
    builder.pass_name = std::string{ name };
    return builder;
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

  void render_pipeline::set_core_buffer(opt<resource_handle>& handle, const std::string_view name) {
    auto itr = std::ranges::find_if(buffer_resources, [&](const auto& pair) {
      return pair.second.name == name;
    });
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Camera buffer resource [{}] not found in pipeline.", name);
      return;
    }
    handle = itr->second.handle;
  }

  void render_pipeline::validate_pipeline() {
    bool has_material_buffer = material_buffer_handle.has_value();
    bool has_model_buffer = model_buffer_handle.has_value();
    bool has_point_light_buffer = point_light_buffer_handle.has_value();
    bool has_direction_light_buffer = direction_light_buffer_handle.has_value();
    bool has_camera_buffer = camera_buffer_handle.has_value();

    if (!has_material_buffer) {
      CORE_LOG_ERROR("Render pipeline must have a material buffer.");
    }
    if (!has_model_buffer) {
      CORE_LOG_ERROR("Render pipeline must have a model buffer.");
    }
    if (!has_point_light_buffer) {
      CORE_LOG_ERROR("Render pipeline must have a point light buffer.");
    }
    if (!has_direction_light_buffer) {
      CORE_LOG_ERROR("Render pipeline must have a directional light buffer.");
    }
    if (!has_camera_buffer) {
      CORE_LOG_ERROR("Render pipeline must have a camera buffer.");
    }
    if (!(has_material_buffer && has_model_buffer && has_point_light_buffer && has_direction_light_buffer && has_camera_buffer)) {
      valid = false;
      CORE_LOG_ERROR("Render pipeline is invalid due to missing buffer bindings.");
    } else {
      valid = graph->is_valid();
      if (!valid) {
        CORE_LOG_ERROR("Render pipeline has necessary resources but render graph is not valid!");
      }
    }
  }

}  // namespace other