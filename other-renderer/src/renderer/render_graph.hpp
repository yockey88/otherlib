/**
 * \file renderer/render_graph.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_GRAPH_HPP
#define OTHER_RENDERER_RENDER_GRAPH_HPP

#include <cstdint>
#include <map>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "data-structures/graph.hpp"

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
      ostd::map<natural_t, frame_node> nodes;
      ostd::map<natural_t, std::set<natural_t>> edges;
    };

    using pass_executor = std::function<void(pass_context&)>;

    struct pass_builder {
      pass_builder(render_graph& graph, render_pass& pass)
          : pass(pass), graph(graph) {}

      pass_builder& set_clear_color(const glm::vec4& clear_color);
      pass_builder& texture_resource(resource_handle handle, const std::string_view uname, natural_t slot, framebuffer::attachment_type type, access_flags flags = READ_WRITE, uint32_t mip_level = 0);
      pass_builder& buffer_resource(resource_handle handle, uint32_t binding, access_flags flags = READ_WRITE);
      pass_builder& execution_callback(pass_executor&& executor, void* user_data = nullptr);
      pass_builder& depends_on(const std::string_view pass_name);
      pass_builder& add_uniform(const std::string_view name, const value& val);
      render_graph& end_pass();

      render_pass& pass;

     private:
      render_graph& graph;

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

    pass_builder start_pass(const std::string_view name, opt<resource_handle> shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer = true, uint32_t samples = 1);

    bool is_valid() const { return graph_valid; }

    const ostd::map<natural_t, pass>& get_passes() const { return passes; }
    const ostd::map<natural_t, pass_executor>& get_executors() const { return executors; }
    opt<resource_handle> get_output_texture() const { return output_texture_handle; }

    graph& get_graph() { return pass_graph; }
    const graph& get_graph() const { return pass_graph; }
    const std::span<const natural_t> get_topological_sort() const { return topological_sort; }

    renderer* get_renderer() { return renderer_ptr; }

    void replace_texture_resource(resource_handle old_handle, resource_handle new_handle);
    void replace_buffer_resource(resource_handle old_handle, resource_handle new_handle);
    void replace_pass_shader(resource_handle old_handle, resource_handle new_handle);

   private:
    friend struct pass_builder;

    renderer* renderer_ptr = nullptr;
    bool graph_valid = false;

    ostd::map<natural_t, pass> passes;
    ostd::map<natural_t, pass_executor> executors;
    opt<resource_handle> output_texture_handle = std::nullopt;

    // graph<render_pass> pass_graph;
    graph pass_graph;
    ostd::vector<natural_t> topological_sort;

    pass& create_pass(render_pass::type rptype);

    void build_graph();

    ostd::vector<natural_t> get_topological_sort(const graph& g);
    void dump_pass_graph(const graph& g);
    void log_topo_sort_error(const graph& g, const ostd::map<natural_t, uint32_t>& remaining_in_degrees);

    natural_t next_pass_id = 0;
    inline natural_t get_next_pass_id() { return ++next_pass_id; }
  };

}  // namespace other

OTHER_REFLECT(
  other::render_pass,
  field(id, other::attr::serializable()))

// OTHER_REFLECT(
//   other::render_graph
// )

#endif  // OTHER_RENDERER_RENDER_GRAPH_HPP