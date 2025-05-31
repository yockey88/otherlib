/**
 * \file renderer/render_graph.hpp
 **/
#ifndef OTHER_RENDERER_RENDER_GRAPH_HPP
#define OTHER_RENDERER_RENDER_GRAPH_HPP

#include <cstdint>
#include <map>

#include <glm/glm.hpp>

namespace other {

  class render_graph;

  struct render_pass {
    natural_t id = 0;
    render_graph* graph = nullptr;

    /// all attachments must be same size, if this is 0, then swapchain size is used
    glm::ivec2 size = { 0, 0 };

    std::vector<natural_t> resources;
    std::vector<uint32_t> access_flags;

    // std::vector<img_attach_info> image_attachments;
    // std::vector<img_resource_handle> image_resources;

    std::vector<int32_t> texture_ids;
    // std::vector<sampler_info> sampler_infos;

    /// callback setup ?
    void* user_data = nullptr;

    /// execute callbacks

    size_t references = 0;

    render_pass(render_graph* graph, natural_t id)
        : id(id), graph(graph) {}
  };

  class render_graph {
   public:
    render_pass& bind_pass(natural_t id, const glm::ivec2& size = { 0, 0 });

   private:
    /// resource handles
    /// resource information
    std::map<natural_t, render_pass> passes;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDER_GRAPH_HPP