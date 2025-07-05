/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "model/model.hpp"
#include "model/vertex.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/renderer_backend.hpp"

#include "SDL3/SDL_mouse.h"

namespace other {

  renderer::renderer()
      : draw_call_pool(true), material_buffer_pool(true), model_buffer_pool(true) {
    material_buffer_handle = gpu_buffer::create("renderer-material_buffer", gpu_buffer::buf_type::UNIFORM_BUFFER, gpu_buffer::usage::DYNAMIC);
    model_buffer_handle = gpu_buffer::create("renderer-model_matrix_buffer", gpu_buffer::buf_type::STORAGE_BUFFER, gpu_buffer::usage::DYNAMIC);
  }

  void renderer::begin_frame(frame_resources* resources) {
    mesh_indices.clear();
    if (resources != nullptr) {
      current_frame_resources = *resources;
    } else {
      current_frame_resources = {
        .model_buffer = model_buffer_handle,
        .material_buffer = material_buffer_handle,
      };
    }

    rendering()->api()->begin_frame();
  }

  void renderer::end_frame() {
    rendering()->api()->end_frame();
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

  void renderer::submit_model(model* draw_model, shader* shader_handle, const gpu::graphics_material& material, const glm::mat4& root_transform) {
    OTHER_ASSERT(draw_model != nullptr, "Draw command model is null");
    OTHER_ASSERT(draw_model->source != nullptr, "Draw command model source is null");

    model_source* source = draw_model->source;
    OTHER_ASSERT(source != nullptr, "Model source is null");

    const std::vector<submesh>& submeshes = source->get_submeshes();
    OTHER_ASSERT(!submeshes.empty(), "Model source has no submeshes");

    const std::vector<uint32_t>& sm_idxs = draw_model->submesh_indices;
    OTHER_ASSERT(!sm_idxs.empty(), "Model has no submeshes");

    for (const auto& sm_idx : sm_idxs) {
      OTHER_ASSERT(sm_idx < submeshes.size(), "Submesh index out of bounds");
      submit_draw_command({
        .draw_model = draw_model,
        .shader_handle = shader_handle,
        .transform = root_transform,
        .material = material,
        .submesh_index = sm_idx,
        .render_state = render_polygon_mode::POLYGON_MODE_FILL,
        .draw_mode = mesh::primitive_type::TRIANGLES,
        .line_thickness = 1.f,
      });
    }
  }

  void renderer::submit_draw_command(const draw_command& command) {
    OTHER_ASSERT(command.draw_model != nullptr, "Draw command model is null");
    OTHER_ASSERT(command.draw_model->source != nullptr, "Draw command model source is null");

    integer_t mesh_index = get_mesh_key_index(command);
    if (mesh_index < 0) {
      return;
    }

    draw_call& call = draw_call_pool[mesh_index];
    if (call.instance_count == 0) {
      call.mesh = rendering()->api()->get_resource_as<mesh>(command.draw_model->source->get_mesh_handle());
      call.shader = command.shader_handle;
      call.submesh_index = command.submesh_index;

      const submesh& submesh = command.draw_model->source->get_submeshes()[command.submesh_index];
      call.vertex_offset = submesh.base_vertex;
      call.vertex_count = submesh.vert_cnt;
      call.index_offset = submesh.base_idx;
      call.index_count = submesh.idx_cnt;

      call.line_thickness = command.line_thickness;

      material_buffer_pool[mesh_index].buffer_data(command.material);
      model_buffer_pool[mesh_index].buffer_data(command.transform);
    } else {
      call.instance_count++;
    }
  }

  void renderer::execute_draw_calls() {
    if (num_draw_calls == 0) {
      return;
    }

    /// upload light buffer and camera buffer data

    gpu_buffer* material_buffer = rendering()->api()->get_resource_as<gpu_buffer>(material_buffer_handle);
    gpu_buffer* model_buffer = rendering()->api()->get_resource_as<gpu_buffer>(model_buffer_handle);
    OTHER_ASSERT(material_buffer != nullptr, "Material buffer is null");
    OTHER_ASSERT(model_buffer != nullptr, "Model buffer is null");

    for (natural_t i = 0; i < num_draw_calls; ++i) {
      draw_call& call = draw_call_pool[i];
      if (call.instance_count == 0) {
        continue;  // Skip empty draw calls
      }

      OTHER_ASSERT(call.mesh != nullptr, "Draw call mesh is null");
      OTHER_ASSERT(call.shader != nullptr, "Draw call shader is null");

      mesh_key& key = mesh_key_pool[i];
      arena_buffer& cpu_material_storage = material_buffer_pool[i];
      arena_buffer& cpu_model_storage = model_buffer_pool[i];

      material_buffer->set_data(cpu_material_storage.data(), cpu_material_storage.size()).finalize_buffer();
      model_buffer->set_data(cpu_model_storage.data(), cpu_model_storage.size()).finalize_buffer();

      rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
    }
  }

  void renderer::render(const render_graph& graph) {
    if (graph.passes.empty()) {
      CORE_LOG_ERROR("Render graph has no passes to render.");
      return;
    }

    /// compile graph into executable commands
  }

  integer_t renderer::get_mesh_key_index(const draw_command& cmd) {
    mesh_key key = cmd;
    auto it = mesh_indices.find(key);
    if (it != mesh_indices.end()) {
      return it->second;
    }
    if (num_draw_calls >= memory_pool<draw_call>::kMaxObjects) {
      CORE_LOG_ERROR("Maximum number of draw calls exceeded: {}", memory_pool<draw_call>::kMaxObjects);
      return -1;
    }

    auto [itr, inserted] = mesh_indices.insert({ key, ++num_draw_calls });
    OTHER_ASSERT(inserted, "Failed to insert mesh key into map");

    mesh_key_pool[itr->second] = key;
    draw_call_pool[itr->second] = draw_call();
    model_buffer_pool[itr->second] = arena_buffer();
    material_buffer_pool[itr->second] = arena_buffer();

    return itr->second;
  }

  // std::map<mesh_key, draw_call>::iterator renderer::insert_mesh_key(const mesh_key& key, model* draw_model) {
  //   OTHER_ASSERT(draw_model != nullptr, "Model is null");
  //   OTHER_ASSERT(draw_model->source != nullptr, "Model source is null");

  //   model_source* model_source = draw_model->source;

  //   const std::vector<submesh>& submeshes = draw_model->source->get_submeshes();
  //   OTHER_ASSERT(!submeshes.empty(), "Model source has no submeshes");

  //   const std::vector<uint32_t>& sm_idxs = draw_model->submesh_indices;
  //   OTHER_ASSERT(!sm_idxs.empty(), "Model has no submeshes");
  //   OTHER_ASSERT(sm_idxs.size() < submeshes.size(), "Submesh indices size exceeds submeshes size");
  //   OTHER_ASSERT(key.submesh_index < submeshes.size(), "Submesh index out of bounds");

  //   const submesh& submesh = submeshes[key.submesh_index];
  //   draw_call msl = {
  //     .mesh = rendering()->api()->get_resource_as<mesh>(model_source->get_mesh_handle()),

  //     // .cpu_model_storage = arena_buffer(),
  //     // .cpu_material_storage = arena_buffer(),

  //     .base_instance = 0,
  //     .instance_count = 0,

  //     .vertex_offset = submesh.base_vertex,
  //     .vertex_count = submesh.vert_cnt,

  //     .index_offset = submesh.base_idx,
  //     .index_count = submesh.idx_cnt,

  //     .shader = rendering()->api()->get_resource_as<shader>(key.model_source_handle),

  //     .line_thickness = key.line_thickness,
  //   };

  //   return main_command_list.insert({ key, std::move(msl) }).first;
  // }

  renderer_backend* renderer::rendering() {
    return subsystem<renderer_backend>::get();
  }

}  // namespace other