/**
 * \file renderer/render_graph.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_GRAPH_HPP
#define OTHER_RENDERER_RENDER_GRAPH_HPP

#include <cstdint>
#include <map>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "renderer/renderer_resource.hpp"
#include "renderer/shader.hpp"

namespace other {

  class renderer;

  class render_graph;

  struct render_pass {
    OTHER_REFLECTABLE(render_pass);

    natural_t id = 0;
    render_graph* graph = nullptr;

    /// all attachments must be same size, if this is 0, then swapchain size is used
    glm::ivec2 size = { 0, 0 };

    resource_handle shader_handle;

    std::vector<resource_handle> color_attachments;
    std::vector<uint32_t> color_attachment_binding_points;

    std::vector<resource_handle> buffer_resources;
    std::vector<uint32_t> buffer_resource_binding_points;

    // std::vector<sampler_info> sampler_infos;

    /// callback shtuff?
    void (*execute_callback)(renderer&, void*) = nullptr;
    void* user_data = nullptr;

    render_pass() = default;
    render_pass(render_graph* graph, natural_t id)
        : id(id), graph(graph) {}
  };

  class render_graph {
    OTHER_REFLECTABLE(render_graph);

   public:
    struct node {
      OTHER_REFLECTABLE(node);

      natural_t id = 0;
      std::vector<resource_handle> input_resources;
      std::vector<resource_handle> output_resources;

      node() = default;
      node(natural_t id, const std::vector<resource_handle>& input_reources, const std::vector<resource_handle>& output_resources)
          : id(id), input_resources(input_reources), output_resources(output_resources) {}
    };

    struct pass_builder {
      natural_t id = 0;

      pass_builder(render_pass& pass)
          : pass(pass) {}

      pass_builder& add_color_attachment(resource_handle texture_id, uint32_t binding);
      pass_builder& use_buffer_resource(resource_handle buffer_id, uint32_t binding);

      template <typename Fn>
        requires std::invocable<Fn, renderer&, void*>
      pass_builder& bind_execute_callback(Fn&& callback, void* user_data = nullptr) {
        pass.execute_callback = std::forward<Fn>(callback);
        pass.user_data = user_data;
        return *this;
      }

      render_graph& end_pass();

     private:
      render_pass& pass;
    };

    pass_builder start_pass(natural_t id, const glm::ivec2& size, resource_handle shader_handle);
    render_graph& add_buffer_resource(resource_handle buffer_id, uint32_t binding);

   private:
    friend class renderer;

    render_pass* current_pass = nullptr;

    std::map<natural_t, node> nodes;
    std::map<natural_t, std::vector<natural_t>> edges;

    std::map<natural_t, render_pass> passes;
  };

}  // namespace other

OTHER_REFLECT(
  other::render_pass,
  field(id, other::attr::serializable())
)

OTHER_REFLECT(
  other::render_graph
)

OTHER_REFLECT(
  other::render_graph::node
)

#endif  // OTHER_RENDERER_RENDER_GRAPH_HPP