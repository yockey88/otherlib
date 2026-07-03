/**
 * \file renderer/render_stream_registry.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RENDER_STREAM_REGISTRY_HPP
#define OTHER_RENDERER_RENDERER_RENDER_STREAM_REGISTRY_HPP

#include "core/fnv.hpp"

#include "gpu_resource/mesh.hpp"

namespace other {

  class renderer;

  struct render_stream_recipe {
    std::string shader;  // pipeline shader name to use
    mesh::primitive_type topology;
    std::vector<vertex_attribute> vertex_layout;
  };

  struct render_stream_definition {
    std::string name;
    size_t element_size;
    size_t max_per_frame;
    render_stream_recipe draw_recipe;
  };

  class render_stream_registry {
   public:
    void register_stream(std::string_view name, render_stream_definition defn);
    const render_stream_definition* find(std::string_view name) const;

    std::map<natural_t, render_stream_definition>& entries() { return defs; }
    const std::map<natural_t, render_stream_definition>& entries() const { return defs; }

    void clear() { defs.clear(); }

   private:
    std::map<natural_t, render_stream_definition> defs;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDER_STREAM_REGISTRY_HPP