/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include <SDL3/SDL_mouse.h>

#include "core/defines.hpp"
#include "thread/thread_safety.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  void renderer::begin_frame(render_data* data) {
    ASSERT_MAIN_THREAD();
    if (data != nullptr) {
      scene_data = data;
      rendering()->api()->set_clear_color(data->clear_color);
    }
    rendering()->api()->begin_frame();
  }

  void renderer::render() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("renderer::render");
    for (const auto& [id, pl] : pipelines) {
      if (pl->is_valid()) {
        /// necessary to save here when pipeline calls execute_draw_calls
        current_frame_resources = pl->get_frame_resources();
        pl->prepare_frame(scene_data);
        pl->render_frame(this);
      }
    }
  }

  void renderer::end_frame() {
    ASSERT_MAIN_THREAD();
    rendering()->api()->end_frame();
    scene_data = nullptr;
  }

  opt<resource_handle> renderer::get_pipeline_output(const std::string_view pipeline_name) const {
    ASSERT_MAIN_THREAD();
    uint64_t hash = FNV(pipeline_name);
    auto itr = pipelines.find(hash);
    if (itr == pipelines.end()) {
      CORE_LOG_ERROR("Pipeline with name [{}] not found.", pipeline_name);
      return std::nullopt;
    }

    render_pipeline* pipeline = itr->second;
    if (!pipeline->is_valid()) {
      CORE_LOG_ERROR("Pipeline [{}] is not valid.", pipeline_name);
      return std::nullopt;
    }

    return pipeline->get_screen_texture();
  }

  void renderer::begin_ui_frame() {
    ASSERT_MAIN_THREAD();
    rendering()->api()->begin_ui_frame();
  }

  void renderer::end_ui_frame() {
    ASSERT_MAIN_THREAD();
    rendering()->api()->end_ui_frame();
  }

  glm::ivec2 renderer::get_window_size() {
    ASSERT_MAIN_THREAD();
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      CORE_LOG_ERROR("SDL window handle is null, cannot get window size.");
      return { 0, 0 };
    }

    int width, height;
    bool success = SDL_GetWindowSize(window, &width, &height);
    if (!success) {
      CORE_LOG_ERROR("Failed to get window size: {}", SDL_GetError());
      return { 0, 0 };
    }

    return { width, height };
  }

  void renderer::set_clear_color(const glm::vec4& color) {
    ASSERT_MAIN_THREAD();
    rendering()->api()->set_clear_color(color);
  }

  glm::vec2 renderer::get_mouse_position() {
    ASSERT_MAIN_THREAD();
    SDL_Window* window = rendering()->api()->window_handle();
    if (window == nullptr) {
      return {};
    }

    real_t x, y;
    SDL_MouseButtonFlags _ = SDL_GetMouseState(&x, &y);
    return { x, y };
  }

  resource_handle renderer::create_resource(const std::string& name, resource_type type) {
    ASSERT_MAIN_THREAD();
    return rendering()->api()->create_resource(name, type);
  }

  void renderer::destroy_resource(const resource_handle& handle) {
    ASSERT_MAIN_THREAD();
    rendering()->api()->destroy_resource(handle);
  }

  bool renderer::resource_exists(const resource_handle& handle) {
    ASSERT_MAIN_THREAD();
    return rendering()->api()->resource_exists(handle);
  }

  void renderer::remove_pipeline(const std::string_view name) {
    ASSERT_MAIN_THREAD();
    uint64_t hash = FNV(name);
    auto itr = pipelines.find(hash);
    if (itr == pipelines.end()) {
      CORE_LOG_ERROR("Pipeline with name [{}] not found.", name);
      return;
    }

    render_pipeline* pipeline = itr->second;
    pipeline->shutdown_pipeline();
    arena_allocator<render_pipeline>{}.free(pipeline);
    pipelines.erase(itr);
  }

  void renderer::execute_draw_calls(render_graph::node* current_node) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(current_node != nullptr, "Current node must not be null.");

    if (scene_data == nullptr || scene_data->draw_calls.empty()) {
      return;
    }

    PROFILE_SECTION("renderer::execute_draw_calls");

    opt<resource_handle> material_buffer_handle = current_frame_resources.find(resource_tag(resource_tag::kMaterialTag));
    opt<resource_handle> model_buffer_handle = current_frame_resources.find(resource_tag(resource_tag::kModelTag));
    opt<resource_handle> bone_buffer_handle = current_frame_resources.find(resource_tag(resource_tag::kBoneTag));

    gpu_buffer* material_buffer = nullptr;
    gpu_buffer* model_buffer = nullptr;
    gpu_buffer* bone_buffer = nullptr;

    if (material_buffer_handle.has_value()) {
      material_buffer = rendering()->api()->get_resource_as<gpu_buffer>(*material_buffer_handle);
      OTHER_ASSERT(material_buffer != nullptr, "Material buffer resource handle is invalid.");
    }
    if (model_buffer_handle.has_value()) {
      model_buffer = rendering()->api()->get_resource_as<gpu_buffer>(*model_buffer_handle);
      OTHER_ASSERT(model_buffer != nullptr, "Model buffer resource handle is invalid.");
    }
    if (bone_buffer_handle.has_value()) {
      bone_buffer = rendering()->api()->get_resource_as<gpu_buffer>(*bone_buffer_handle);
      OTHER_ASSERT(bone_buffer != nullptr, "Bone buffer resource handle is invalid.");
    }

    for (natural_t i = 0; i < scene_data->num_draw_calls; ++i) {
      draw_call& call = scene_data->draw_calls[i];
      if (call.instance_count == 0) {
        continue;
      }

      mesh_key& key = scene_data->mesh_keys[i];
      gpu::graphics_material_buffer& cpu_material_storage = scene_data->material_buffers[i];
      gpu::model_matrix_buffer& cpu_model_storage = scene_data->model_buffers[i];
      gpu::bone_matrix_buffer& bone_buffer_data = scene_data->bone_buffers[i];

      if (material_buffer != nullptr) {
        material_buffer
          ->set_data(&cpu_material_storage, sizeof(gpu::graphics_material_buffer))
          .finalize_buffer();
      }
      if (model_buffer != nullptr) {
        model_buffer
          ->set_data(&cpu_model_storage, sizeof(gpu::model_matrix_buffer))
          .finalize_buffer();
      }
      if (bone_buffer != nullptr) {
        bone_buffer
          ->set_data(&bone_buffer_data, sizeof(gpu::bone_matrix_buffer))
          .finalize_buffer();
      }

      rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
    }
  }

  renderer_backend* renderer::rendering() {
    ASSERT_MAIN_THREAD();
    return subsystem<renderer_backend>::get();
  }

}  // namespace other