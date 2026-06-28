/**
 * \file renderer/renderer.cpp
 **/
#include "renderer/renderer.hpp"

#include <SDL3/SDL_mouse.h>

#include "core/defines.hpp"
#include "thread/thread_safety.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/frame_node.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/render_pipeline.hpp"
#include "renderer/renderer_backend.hpp"

namespace other {

  void renderer::initialize_pass_resolver(pass_executor_resolver* resolver) {
    // can be null
    pass_exec_resolver = resolver;
  }

  void renderer::rebuild_pipeline(const std::string_view pipeline_name) {
    auto itr = pipelines.find(FNV(pipeline_name));
    if (itr == pipelines.end()) {
      CORE_LOG_ERROR("Pipeline with name [{}] not found. Cannot rebuild pipeline.", pipeline_name);
      return;
    }
    if (itr->second == nullptr) {
      CORE_LOG_ERROR("Pipeline with name [{}] is null. Cannot rebuild pipeline.", pipeline_name);
      return;
    }
    itr->second->rebuild();
  }

  void renderer::begin_frame(render_data* data) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("renderer::begin_frame");
    if (data != nullptr) {
      scene_data = data;
      rendering()->api()->set_clear_color(data->clear_color);
    }
    rendering()->api()->begin_frame();
  }

  void renderer::bind_frame_bindings(const render_data& data) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("renderer::bind_frame_bindings");
    for (auto& [_, pl] : pipelines) {
      if (!pl->is_valid()) {
        continue;
      }
      pl->bind_frame_resources(data);
    }
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
    PROFILE_SECTION("renderer::end_frame");
    rendering()->api()->end_frame();
    scene_data = nullptr;
  }

  ImTextureID renderer::get_texture_id(const std::string_view pipeline, const std::string_view name) {
    auto itr = pipelines.find(FNV(pipeline));
    if (itr == pipelines.end()) {
      CORE_LOG_ERROR("Pipeline with name [{}] not found. Cannot get texture ID for resource [{}].", pipeline, name);
      return 0;
    }
    if (itr->second == nullptr) {
      CORE_LOG_ERROR("Pipeline with name [{}] is null. Cannot get texture ID for resource [{}].", pipeline, name);
      return 0;
    }
    return itr->second->get_texture_id(name);
  }

  opt<resource_handle> renderer::find_texture_resource(const std::string_view name) const {
    for (auto pl : pipelines | std::views::values) {
      OTHER_ASSERT(pl != nullptr, "Null pipeline found in renderer pipelines.");
      if (auto handle = pl->find_texture_by_name(name); handle.has_value()) {
        return handle;
      }
    }
    return {};
  }

  opt<resource_handle> renderer::find_buffer_resource(const std::string_view name) const {
    for (auto pl : pipelines | std::views::values) {
      OTHER_ASSERT(pl != nullptr, "Null pipeline found in renderer pipelines.");
      if (auto handle = pl->find_buffer_by_name(name); handle.has_value()) {
        return handle;
      }
    }
    return {};
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

  resource_handle renderer::get_or_create_debug_stream_mesh(std::string_view stream_name, const debug_stream_recipe& recipe) {
    natural_t key = FNV(stream_name);
    if (auto itr = debug_stream_meshes.find(key); itr != debug_stream_meshes.end()) {
      return itr->second;
    }
    auto handle = create_resource(std::format("__debug.mesh.{}", stream_name), resource_type::MESH);
    auto& m = get_resource<mesh>(handle);
    m.set_primitive_type(recipe.topology);
    for (const auto& attr : recipe.vertex_layout) {
      m.add_attribute(attr.name, attr.type, attr.size, attr.offset);
    }
    debug_stream_meshes.insert({ key, handle });
    return handle;
  }

  opt<resource_handle> renderer::get_debug_stream_shader_handle(std::string_view shader_name) {
    natural_t key = FNV(shader_name);
    if (auto itr = debug_stream_shaders.find(key); itr != debug_stream_shaders.end()) {
      return itr->second;
    }
    CORE_LOG_ERROR("renderer: debug stream shader '{}' not registered before first draw", shader_name);
    return std::nullopt;
  }

  void renderer::begin_ui_frame() {
    ASSERT_MAIN_THREAD();
    rendering()->api()->begin_ui_frame();
  }

  void renderer::end_ui_frame() {
    ASSERT_MAIN_THREAD();
    rendering()->api()->end_ui_frame();
  }

  bool renderer::executor_resolves(const std::string_view name) const {
    if (pass_exec_resolver == nullptr) {
      return false;
    }
    return pass_exec_resolver->can_resolve_executor(name);
  }

  bool renderer::frame_binder_resolves(const resource_tag& tag) const {
    if (pass_exec_resolver == nullptr) {
      return false;
    }
    return pass_exec_resolver->can_resolve_frame_binder(tag);
  }

  bool renderer::draw_binder_resolves(const resource_tag& tag) const {
    if (pass_exec_resolver == nullptr) {
      return false;
    }
    return pass_exec_resolver->can_resolve_draw_binder(tag);
  }

  bool renderer::instance_binder_resolves(const resource_tag& tag) const {
    if (pass_exec_resolver == nullptr) {
      return false;
    }
    return pass_exec_resolver->can_resolve_instance_binder(tag);
  }

  render_graph::pass_executor renderer::attempt_executor_resolution(const std::string_view name, const pipeline_pass_definition& def, render_pipeline* pl) {
    OTHER_ASSERT(pl != nullptr, "Pipeline can not be null while resolving a pass executor!");

    if (pass_exec_resolver == nullptr) {
      CORE_LOG_ERROR("No pass executor resolver registered! Cannot resolve executor for pass with target [{}]", name);
      return nullptr;
    }

    return pass_exec_resolver->resolve_executor(name, def, pl);
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

  void renderer::draw_mesh(const resource_handle& mesh_handle) {
    ASSERT_MAIN_THREAD();
    mesh* m = rendering()->api()->get_resource_as<mesh>(mesh_handle);
    if (m == nullptr) {
      CORE_LOG_ERROR("Mesh resource with handle {} not found.", mesh_handle.id);
      return;
    }
    m->draw();
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

  void renderer::execute_draw_calls(frame_node* current_node) {
    OTHER_ASSERT(current_node != nullptr, "Current node must not be null.");
    ASSERT_MAIN_THREAD();
    if (scene_data == nullptr || scene_data->draw_calls.empty()) {
      return;
    }
    PROFILE_SECTION("renderer::execute_draw_calls");

    auto& api = rendering()->api();
    OTHER_ASSERT(api != nullptr, "Rendering API is null in execute_draw_calls.");

    render_pipeline* pl = get_pass_pipeline(current_node->pass->id);
    OTHER_ASSERT(pl != nullptr, "execute_draw_calls: no pipeline owns pass id {}", current_node->pass->id);

    pass_runtime& runtime = pl->get_pass_runtime(current_node->pass->id);
    for (natural_t i = 0; i < scene_data->num_draw_calls; ++i) {
      const draw_call& call = scene_data->draw_calls[i];
      if (call.instance_count == 0) {
        continue;
      }

      pl->bind_draw_resources(runtime, *scene_data, i);

      const auto& key = scene_data->mesh_keys[i];
      api->execute_draw_call(key.render_state, key.draw_mode, call);
    }
  }

  renderer_backend* renderer::rendering() {
    ASSERT_MAIN_THREAD();
    return subsystem<renderer_backend>::get();
  }

  render_pipeline* renderer::get_pass_pipeline(natural_t pass_id) const {
    for (const auto& [_, pl] : pipelines) {
      if (!pl->is_valid()) {
        continue;
      }
      if (pl->has_pass(pass_id)) {
        return pl;
      }
    }
    return nullptr;
  }

}  // namespace other