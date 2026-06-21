/**
 * \file renderer/render_graph.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_GRAPH_HPP
#define OTHER_RENDERER_RENDER_GRAPH_HPP

#include <cstdint>
#include <map>

#include <glm/glm.hpp>

#include "core/defines.hpp"

#include "gpu_resource/framebuffer.hpp"
#include "gpu_resource/renderer_resource.hpp"
#include "renderer/frame_node.hpp"
#include "renderer/pass_context.hpp"
#include "renderer/render_pass.hpp"

namespace other {

  class render_graph {
   public:
    /// \todo can we use the graph structure from core/graph.hpp instead?
    ///        this has special implementation considerations because of the
    ///        rendering passes and their resources but maybe we can still do it?
    struct graph {
      std::map<natural_t, frame_node> nodes;
      std::map<natural_t, std::vector<natural_t>> edges;
    };

    using pass_executor = std::function<void(pass_context&)>;

    struct pass_builder {
      pass_builder(render_graph& graph, render_pass& pass)
          : graph(graph), pass(pass) {}

      pass_builder& set_clear_color(const glm::vec4& clear_color);
      pass_builder& texture_resource(resource_handle handle, natural_t slot, framebuffer::attachment_type type, access_flags flags = READ_WRITE, uint32_t mip_level = 0);
      pass_builder& buffer_resource(resource_handle handle, uint32_t binding, access_flags flags = READ_WRITE);
      pass_builder& execution_callback(pass_executor&& executor, void* user_data = nullptr);
      render_graph& end_pass();

     private:
      render_graph& graph;
      render_pass& pass;

      natural_t curr_texture_id = 0;
      natural_t get_next_texture_id() {
        return curr_texture_id++;
      }

      natural_t curr_buffer_id = 0;
      natural_t get_next_buffer_id() {
        return curr_buffer_id++;
      }
    };
    struct pass {
      render_pass::type type;
      render_pass pass;
    };

    render_graph(renderer* renderer_ptr)
        : renderer_ptr(renderer_ptr) {}
    ~render_graph();

    render_graph& start_pipeline();
    void end_pipeline();

    pass_builder start_pass(const std::string_view name, opt<resource_handle> shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer = true);

    bool is_valid() const { return graph_valid; }

    const std::map<natural_t, pass>& get_passes() const { return passes; }
    const std::map<natural_t, pass_executor>& get_executors() const { return executors; }
    opt<resource_handle> get_output_texture() const { return output_texture_handle; }

    graph& get_graph() { return pass_graph; }
    const graph& get_graph() const { return pass_graph; }
    const std::vector<natural_t>& get_topological_sort() const { return topological_sort; }

    renderer* get_renderer() { return renderer_ptr; }

   private:
    friend struct pass_builder;

    renderer* renderer_ptr = nullptr;
    bool graph_valid = false;

    std::map<natural_t, pass> passes;
    std::map<natural_t, pass_executor> executors;
    opt<resource_handle> output_texture_handle = std::nullopt;

    graph pass_graph;
    std::vector<natural_t> topological_sort;

    pass& create_pass(render_pass::type rptype);

    void build_graph();
    std::vector<natural_t> get_topological_sort(const graph& g);

    natural_t next_pass_id = 0;
    inline natural_t get_next_pass_id() { return ++next_pass_id; }

    natural_t next_node_id = 0;
    inline natural_t get_next_node_id() { return next_node_id++; }
  };

}  // namespace other

OTHER_REFLECT(
  other::render_pass,
  field(id, other::attr::serializable()))

// OTHER_REFLECT(
//   other::render_graph
// )

#endif  // OTHER_RENDERER_RENDER_GRAPH_HPP