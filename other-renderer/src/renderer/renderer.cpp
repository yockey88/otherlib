/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_mouse.h"

namespace other {

  renderer::renderer() {}

  void renderer::submit_render_data(render_data* data) {
    OTHER_ASSERT(data != nullptr, "Render data must not be null");
    scene_data = data;
  }

  void renderer::begin_frame(frame_resources* resources) {
    OTHER_ASSERT(resources != nullptr, "Resources must be provided");
    current_frame_resources = *resources;

    rendering()->api()->begin_frame();
  }

  void renderer::end_frame() {
    rendering()->api()->end_frame();
    scene_data = nullptr;
  }

  void renderer::execute_frame(const render_graph& graph, frame_resources* resources) {
    PROFILE_SECTION("renderer::execute");
    begin_frame(resources);
    render(graph);
    end_frame();
  }

  void renderer::begin_ui_frame() {
    rendering()->api()->begin_ui_frame();
  }

  void renderer::end_ui_frame() {
    rendering()->api()->end_ui_frame();
  }

  glm::ivec2 renderer::get_window_size() {
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      CORE_LOG_ERROR("SDL window handle is null, cannot get window size.");
      return { 0, 0 };
    }

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return { width, height };
  }

  void renderer::set_clear_color(const glm::vec4& color) {
    rendering()->api()->set_clear_color(color);
  }

  glm::vec2 renderer::get_mouse_position() {
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      return {};
    }

    real_t x, y;
    SDL_MouseButtonFlags _ = SDL_GetMouseState(&x, &y);
    return { x, y };
  }

  resource_handle renderer::create_resource(const std::string& name, resource_type type) {
    return rendering()->api()->create_resource(name, type);
  }

  void renderer::destroy_resource(const resource_handle& handle) {
    rendering()->api()->destroy_resource(handle);
  }

  void renderer::execute_draw_calls() {
    if (scene_data == nullptr || scene_data->draw_calls.empty()) {
      CORE_LOG_WARN("No render data submitted for this frame, skipping draw calls.");
      return;
    }
    PROFILE_SECTION("renderer::execute_draw_calls");

    gpu_buffer* material_buffer = rendering()->api()->get_resource_as<gpu_buffer>(current_frame_resources.material_buffer);
    gpu_buffer* model_buffer = rendering()->api()->get_resource_as<gpu_buffer>(current_frame_resources.model_buffer);
    OTHER_ASSERT(material_buffer != nullptr, "Material buffer is null");
    OTHER_ASSERT(model_buffer != nullptr, "Model buffer is null");

    for (natural_t i = 0; i < scene_data->num_draw_calls; ++i) {
      draw_call& call = scene_data->draw_calls[i];
      if (call.instance_count == 0) {
        continue;
      }

      mesh_key& key = scene_data->mesh_keys[i];
      gpu::graphics_material_buffer& cpu_material_storage = scene_data->material_buffers[i];
      gpu::model_matrix_buffer& cpu_model_storage = scene_data->model_buffers[i];

      material_buffer->set_shader_resource(0, call.shader_handle)
        .set_data(&cpu_material_storage, sizeof(gpu::graphics_material_buffer))
        .finalize_buffer();
      model_buffer->set_shader_resource(1, call.shader_handle)
        .set_data(&cpu_model_storage, sizeof(gpu::model_matrix_buffer))
        .finalize_buffer();

      rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
    }
  }

  void renderer::render(const render_graph& graph) {
    if (!graph.is_valid()) {
      CORE_LOG_ERROR("Render graph has no passes to render.");
      return;
    }

    const auto& g = graph.get_graph();
    const auto& execs = graph.get_executors();
    for (const natural_t id : graph.get_topological_sort()) {
      auto node_atr = g.nodes.find(id);
      OTHER_ASSERT(node_atr != g.nodes.end(), "Node with id {} not found in graph.", id);

      const auto& n = node_atr->second;
      const auto* pass = n.pass;
      auto itr = execs.find(pass->id);
      OTHER_ASSERT(itr != execs.end(), "Executor for pass {} not found.", id);

      if (pass->framebuffer_handle.has_value()) {
        get_resource<framebuffer>(pass->framebuffer_handle.value()).bind();
      }
      get_resource<shader>(pass->shader_handle).bind();

      for (const auto& [binding_point, buffer] : n.input_buffers) {
        get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(binding_point, pass->shader_handle)
          .bind();
      }
      for (const auto& [binding_point, buffer] : n.output_buffers) {
        get_resource<gpu_buffer>(buffer.handle)
          .set_shader_resource(binding_point, pass->shader_handle)
          .bind();
      }
      for (const auto& [slot, tex] : n.input_textures) {
        get_resource<texture>(tex.handle).bind(slot);
      }

      /// set other pipeline state options here
      itr->second.operator()(*this, &n, pass->user_data);

      for (const auto& [slot, tex] : n.input_textures) {
        get_resource<texture>(tex.handle).unbind(slot);
      }
      for (const auto& [binding_point, buffer] : n.output_buffers) {
        get_resource<gpu_buffer>(buffer.handle).unbind();
      }
      for (const auto& [binding_point, buffer] : n.input_buffers) {
        get_resource<gpu_buffer>(buffer.handle).unbind();
      }

      get_resource<shader>(pass->shader_handle).unbind();
      if (pass->framebuffer_handle.has_value()) {
        get_resource<framebuffer>(pass->framebuffer_handle.value()).unbind();
      }
    }
  }

  renderer_backend* renderer::rendering() {
    return subsystem<renderer_backend>::get();
  }

}  // namespace other