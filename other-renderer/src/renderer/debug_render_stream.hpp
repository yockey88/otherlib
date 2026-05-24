/**
 * \file renerer/debug_render_stream.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP
#define OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP

#include "gpu_resource/mesh.hpp"

namespace other {

  struct debug_stream_recipe {
    std::string shader;  // pipeline shader name to use
    mesh::primitive_type topology;
    std::vector<vertex_attribute> vertex_layout;
  };

  struct debug_stream_definition {
    size_t element_size;
    size_t max_per_frame;
    debug_stream_recipe draw_recipe;
  };

  class debug_stream_registry {
   public:
    void register_stream(std::string_view name, debug_stream_definition defn);
    const debug_stream_definition* find(std::string_view name) const;

   private:
    std::map<natural_t, debug_stream_definition> defs;
  };

  class debug_streams {
   public:
    template <typename T>
    void submit(std::string_view stream_name, const T& item);

    std::span<const uint8_t> view(std::string_view stream_name) const;
    size_t count(std::string_view stream_name) const;
    size_t element_size(std::string_view stream_name) const;

   private:
    struct stream_storage {
      size_t element_size;
      size_t count;
      std::vector<uint8_t> bytes;
    };
    std::map<natural_t, stream_storage> storages;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP