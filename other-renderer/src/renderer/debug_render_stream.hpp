/**
 * \file renerer/debug_render_stream.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP
#define OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP

#include "core/fnv.hpp"

#include "gpu_resource/mesh.hpp"

namespace other {

  class renderer;

  struct debug_stream_recipe {
    std::string shader;  // pipeline shader name to use
    mesh::primitive_type topology;
    std::vector<vertex_attribute> vertex_layout;
  };

  struct debug_stream_definition {
    std::string name;
    size_t element_size;
    size_t max_per_frame;
    debug_stream_recipe draw_recipe;
  };

  class debug_stream_registry {
   public:
    void register_stream(std::string_view name, debug_stream_definition defn);
    const debug_stream_definition* find(std::string_view name) const;

    std::map<natural_t, debug_stream_definition>& entries() { return defs; }
    const std::map<natural_t, debug_stream_definition>& entries() const { return defs; }

   private:
    std::map<natural_t, debug_stream_definition> defs;
  };

  class debug_streams {
   public:
    template <typename T>
    void submit(std::string_view stream_name, const T& item) {
      auto itr = storages.find(FNV(stream_name));
      if (itr == storages.end()) {
        CORE_LOG_ERROR("Debug stream with name [{}] not found. Cannot submit item.", stream_name);
        return;
      }

      const uint8_t* item_bytes = reinterpret_cast<const uint8_t*>(&item);
      itr->second.bytes.append_range(std::span<const uint8_t>(item_bytes, sizeof(T)));
      itr->second.count++;
    }

    void configure_streams(renderer* renderer_ptr, const debug_stream_registry& registry);
    void configure_stream(renderer* renderer_ptr, const debug_stream_definition& defn);

    resource_handle get_mesh_handle(std::string_view stream_name) const;

    std::span<const uint8_t> view(std::string_view stream_name) const;
    size_t count(std::string_view stream_name) const;
    size_t element_size(std::string_view stream_name) const;

    void clear();

   private:
    struct stream_storage {
      size_t element_size;
      size_t max_per_frame = 0;
      size_t count;
      std::vector<uint8_t> bytes;

      resource_handle mesh_handle;
    };
    std::map<natural_t, stream_storage> storages;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP