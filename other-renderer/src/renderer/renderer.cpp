/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_mouse.h"

namespace other {

  void renderer::begin_frame(render_data* data) {
    if (data != nullptr) {
      scene_data = data;
    }
    rendering()->api()->begin_frame();
  }

  void renderer::render() {
    PROFILE_SECTION("renderer::render");
    if (scene_data == nullptr || scene_data->draw_calls.empty()) {
      return;
    }

    for (const auto& [id, pl] : pipelines) {
      if (pl->is_valid()) {
        current_frame_resources = pl->get_frame_resources();
        pl->prepare_frame(&current_frame_resources, scene_data);
        pl->render_frame(this);
      }
    }
  }

  void renderer::end_frame() {
    rendering()->api()->end_frame();
    scene_data = nullptr;
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

  void renderer::remove_pipeline(const std::string_view name) {
    uint64_t hash = FNV(name);
    auto itr = pipelines.find(hash);
    if (itr == pipelines.end()) {
      CORE_LOG_ERROR("Pipeline with name [{}] not found.", name);
      return;
    }

    render_pipeline* pipeline = itr->second;
    pipeline->shutdown_pipeline();
    delete pipeline;
    pipelines.erase(itr);
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

      material_buffer
        ->set_data(&cpu_material_storage, sizeof(gpu::graphics_material_buffer))
        .finalize_buffer();
      model_buffer
        ->set_data(&cpu_model_storage, sizeof(gpu::model_matrix_buffer))
        .finalize_buffer();

      rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
    }
  }

  renderer_backend* renderer::rendering() {
    return subsystem<renderer_backend>::get();
  }

}  // namespace other