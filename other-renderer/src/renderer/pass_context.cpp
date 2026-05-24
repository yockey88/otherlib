/**
 * \file renderer/pass_context.cpp
 **/
#include "renderer/pass_context.hpp"

#include "thread/thread_safety.hpp"

#include "renderer/frame_node.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void pass_diagnostics::mark(std::string_view label) {
  }

  void pass_diagnostics::log_warning(std::string_view warning) {
  }

  pass_context::pass_context(renderer* r, render_pipeline* p, frame_node* n, const render_data* d, frame_binding_view bv, pass_diagnostics* diag)
      : renderer_ptr(r), pipeline(p), node(n), frame_data(d), bindings(bv), diag(diag) {
    OTHER_ASSERT(renderer_ptr != nullptr, "pass_context: renderer is null");
    OTHER_ASSERT(pipeline != nullptr, "pass_context: pipeline is null");
    OTHER_ASSERT(node != nullptr, "pass_context: node is null");
    OTHER_ASSERT(node->pass != nullptr, "pass_context: node->pass is null");
  }

  // Resource access by logical binding name — validated at compile time
  resource_handle pass_context::binding(std::string_view logical_name) const {
    for (size_t i = 0; i < bindings.defs.size(); ++i) {
      if (bindings.defs[i].name == logical_name) {
        OTHER_ASSERT(i < bindings.per_frame_handles.size(), "pass_context::binding('{}'): handle index {} out of range", logical_name, i);
        return bindings.per_frame_handles[i];
      }
    }
    OTHER_ASSERT(false, "pass_context::binding('{}'): not declared on pass '{}'", logical_name, node->pass->name);
    return resource_handle{};
  }

  shader* pass_context::shader_for_pass() const {
    if (!node->pass->shader_handle.has_value()) {
      return nullptr;
    }
    return &renderer_ptr->get_resource<shader>(*node->pass->shader_handle);
  }
  renderer& pass_context::get_renderer() const {
    OTHER_ASSERT(renderer_ptr != nullptr, "pass_context: renderer is null");
    return *renderer_ptr;
  }

  const render_data& pass_context::get_frame_data() const {
    OTHER_ASSERT(frame_data != nullptr, "Frame data is null in pass context.");
    return *frame_data;
  }

  void pass_context::draw_quad() {
    ASSERT_MAIN_THREAD();
    auto handle = pipeline->get_quad_mesh_handle();
    auto& mesh_resource = renderer_ptr->get_resource<mesh>(handle);
    mesh_resource.draw();
  }

  void pass_context::draw_stream() {
    ASSERT_MAIN_THREAD();
    renderer_ptr->execute_draw_calls(node);
  }

  void pass_context::submit_draw_call(const draw_call& call, const mesh_key& key) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(call.instance_count > 0, "submit_draw_call: instance_count = 0");
    renderer_ptr->rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
  }

  void pass_context::dispatch(const glm::uvec3& groups, shader::compute_barrier_type barrier) {
    ASSERT_MAIN_THREAD();
    auto* sh = shader_for_pass();
    OTHER_ASSERT(sh != nullptr, "pass_context::dispatch: pass '{}' has no shader bound", node->pass->name);
    OTHER_ASSERT(node->pass->pass_type == render_pass::COMPUTE_PASS, "pass_context::dispatch: pass '{}' is not a COMPUTE_PASS", node->pass->name);
    renderer_ptr->rendering()->api()->dispatch_shader(*node->pass->shader_handle, glm::ivec3(groups), barrier);
  }

  void pass_context::draw_debug_stream(std::string_view stream_name, const debug_stream_definition& def, std::span<const uint8_t> data, size_t count) {
    ASSERT_MAIN_THREAD();
    if (count == 0) return;
    OTHER_ASSERT(data.size() == def.element_size * count, "draw_debug_stream('{}'): data.size()={} but element_size*count={}", stream_name, data.size(), def.element_size * count);

    // Resolve the recipe shader by name once per call. Caches into a stream
    // mesh that lives on the renderer keyed by stream name.
    resource_handle stream_mesh = renderer_ptr->get_or_create_debug_stream_mesh(stream_name, def.draw_recipe);
    auto& mesh_res = renderer_ptr->get_resource<mesh>(stream_mesh);
    mesh_res.upload_vertex_buffer(std::format("{}.vertices", stream_name), count, data.data(), data.size())
      .finalize_mesh();

    // Bind the shader the recipe asked for, then draw.
    auto shader_handle = renderer_ptr->get_debug_stream_shader_handle(def.draw_recipe.shader);
    OTHER_ASSERT(shader_handle.has_value(), "draw_debug_stream('{}'): shader '{}' not loaded", stream_name, def.draw_recipe.shader);

    auto& sh = renderer_ptr->get_resource<shader>(*shader_handle);
    sh.bind();
    renderer_ptr->rendering()->api()->draw_mesh(stream_mesh, def.draw_recipe.topology, count);
    sh.unbind();
  }

}  // namespace other