/**
 * \file renderer/pass_context.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_CONTEXT_HPP
#define OTHER_RENDERER_RENDERER_PASS_CONTEXT_HPP

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/shader.hpp"
#include "renderer/debug_render_stream.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/frame_binding_definition.hpp"
#include "renderer/frame_binding_registry.hpp"
#include "renderer/frame_node.hpp"
#include "renderer/render_pass.hpp"

namespace other {

  struct render_data;
  class renderer;
  class render_pipeline;

  struct frame_binding_view {
    std::span<const frame_binding_definition> defs;
    std::span<const resource_handle> per_frame_handles;
    std::span<const uint32_t> per_draw_offsets;
  };

  struct pass_diagnostics {
    void mark(std::string_view label);
    void log_warning(std::string_view warning);
  };

  class OTHER_API pass_context {
   public:
    pass_context(renderer* r, render_pipeline* p, frame_node* n, const render_data* d, frame_binding_view bv, pass_diagnostics* diag);

    resource_handle binding(std::string_view logical_name) const;
    shader* shader_for_pass() const;

    renderer& get_renderer() const;
    const render_data& get_frame_data() const;

    void draw_quad();
    void draw_stream();
    void submit_draw_call(const draw_call& call, const mesh_key& key);
    void dispatch(const glm::uvec3& groups, shader::compute_barrier_type barrier);
    void draw_debug_stream(std::string_view stream_name, const debug_stream_definition& def, std::span<const uint8_t> data, size_t count);

    template <typename T>
    void emit_debug(std::string_view stream_name, const T& entry) {
    }

    template <typename T>
    void set_uniform(std::string_view name, const T& value) {
      auto* sh = shader_for_pass();
      OTHER_ASSERT(sh != nullptr, "pass_context::set_uniform('{}'): pass '{}' has no shader bound", name, node->pass->name);
      sh->set_uniform(name, value);
    }

   private:
    renderer* renderer_ptr;
    render_pipeline* pipeline;
    frame_node* node;
    const render_data* frame_data;
    frame_binding_view bindings;
    pass_diagnostics* diag;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_CONTEXT_HPP