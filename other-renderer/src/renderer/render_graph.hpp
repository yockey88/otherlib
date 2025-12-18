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

namespace other {

  class renderer;
  class render_graph;
  struct render_pass {
    enum type {
      RENDER_PASS = 0,
      COMPUTE_PASS,
    } pass_type = RENDER_PASS;
    natural_t id = 0;
    opt<resource_handle> framebuffer_handle = std::nullopt;
    opt<resource_handle> shader_handle = {};
    void* user_data = nullptr;

    /// for other dynamic resource binding later
    natural_t next_texture_id = 0;
    natural_t next_buffer_id = 0;

    std::string name;
    glm::ivec2 size = { 0, 0 };
    glm::vec4 clear_color = { 0.2, 0.2, 0.2, 1.0 };

    struct texture_resource {
      framebuffer::attachment_type type;
      natural_t slot;
      access_flags flags;
      resource_handle handle;
    };
    struct buffer_resource {
      natural_t binding_point;
      access_flags flags;
      resource_handle handle;
    };

    std::map<natural_t, texture_resource> texture_resources;
    std::map<natural_t, buffer_resource> buffer_resources;

    void bind_pass(renderer* renderer_ptr);
    void unbind_pass(renderer* renderer_ptr);
  };

  class render_graph {
   public:
    struct node {
      natural_t id;
      render_pass* pass = nullptr;

      std::map<natural_t, render_pass::buffer_resource> input_buffers;
      std::map<natural_t, render_pass::buffer_resource> output_buffers;
      std::map<natural_t, render_pass::texture_resource> input_textures;
      std::map<natural_t, render_pass::texture_resource> output_textures;

      void start_pass(renderer* renderer_ptr) const;
      void end_pass(renderer* renderer_ptr) const;

      bool operator==(const node& other) const { return id == other.id && pass == other.pass; }
    };
    /// \todo can we use the graph structure from core/graph.hpp instead?
    ///        this has special implementation considerations because of the
    ///        rendering passes and their resources but maybe we can still do it?
    struct graph {
      std::map<natural_t, node> nodes;
      std::map<natural_t, std::vector<natural_t>> edges;
    };

    /// \todo finish scripting and use actions:
    ///           using pass_executor = action<renderer&, node*, void*>;
    using pass_executor = std::function<void(renderer& render, node*, void*)>;

    struct pass_builder {
      pass_builder(render_graph& graph, render_pass& pass)
          : graph(graph), pass(pass) {}

      pass_builder& set_clear_color(const glm::vec4& clear_color);
      pass_builder& texture_resource(resource_handle handle, natural_t slot, framebuffer::attachment_type type, access_flags flags = READ_WRITE);
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
  field(id, other::attr::serializable())
)

// OTHER_REFLECT(
//   other::render_graph
// )

#endif  // OTHER_RENDERER_RENDER_GRAPH_HPP