/**
 * \file renderer/pass_context.cpp
 **/
#include "renderer/pass_context.hpp"

#include "thread/thread_safety.hpp"

#include "renderer/frame_node.hpp"
#include "renderer/renderer.hpp"

namespace other {

  void pass_diagnostics::mark(std::string_view label) {
    /// \todo this
  }

  void pass_diagnostics::log_warning(std::string_view warning) {
    /// \todo this
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

  void pass_context::execute_draw_call(const draw_call& call, const mesh_key& key) {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(call.instance_count > 0, "draw_call: instance_count = 0");
    renderer_ptr->rendering()->api()->execute_draw_call(key.render_state, key.draw_mode, call);
  }

  void pass_context::dispatch(const glm::uvec3& groups, shader::compute_barrier_type barrier) {
    ASSERT_MAIN_THREAD();
    auto* sh = shader_for_pass();
    OTHER_ASSERT(sh != nullptr, "pass_context::dispatch: pass '{}' has no shader bound", node->pass->name);
    OTHER_ASSERT(node->pass->pass_type == render_pass::COMPUTE_PASS, "pass_context::dispatch: pass '{}' is not a COMPUTE_PASS", node->pass->name);
    renderer_ptr->rendering()->api()->dispatch_shader(*node->pass->shader_handle, glm::ivec3(groups), barrier);
  }

  void pass_context::draw_debug_vertices(std::string_view stream_name, mesh::primitive_type topology) {
    ASSERT_MAIN_THREAD();
    const render_stream& s = frame_data->debug_data;
    const size_t verts = s.count(stream_name);

    if (verts == 0) {
      return;
    }

    resource_handle m = s.get_mesh_handle(stream_name);
    renderer_ptr->get_resource<mesh>(m)
      .upload_vertex_buffer(stream_name, gpu_buffer::DYNAMIC, (uint32_t)verts, s.view(stream_name).data(), s.view(stream_name).size())
      .finalize_mesh();
    renderer_ptr->rendering()->api()->draw_mesh(m, topology, verts);
  }

  void pass_context::draw_debug_mesh(const debug_mesh_instance& instance) {
    ASSERT_MAIN_THREAD();
    if (!renderer_ptr->resource_exists(instance.mesh_handle)) {
      return;
    }

    shader* sh = shader_for_pass();
    OTHER_ASSERT(sh != nullptr, "draw_debug_mesh: pass '{}' has no shader bound", node->pass->name);
    sh->set_uniform("OE_model", instance.model);
    sh->set_uniform("OE_tint", instance.color);

    auto& api = renderer_ptr->rendering()->api();
    const bool wire = (instance.flags & debug_mesh_instance::DEBUG_MESH_WIREFRAME) != 0;
    api->set_polygon_mode(wire ? POLYGON_MODE_LINE : POLYGON_MODE_FILL);
    if (instance.flags & debug_mesh_instance::DEBUG_MESH_NO_DEPTH) {
      api->set_depth_test(false);
    }

    renderer_ptr->get_resource<mesh>(instance.mesh_handle).draw();

    if (instance.flags & debug_mesh_instance::DEBUG_MESH_NO_DEPTH) {
      api->set_depth_test(true);
    }

    api->set_polygon_mode(POLYGON_MODE_FILL);
  }

}  // namespace other