/**
 * \file renerer/debug_render_stream.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP
#define OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP

#include "core/fnv.hpp"

#include "gpu_resource/mesh.hpp"

namespace other {

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

   private:
    std::map<natural_t, debug_stream_definition> defs;
  };

  class debug_streams {
   public:
    template <typename T>
    void submit(std::string_view stream_name, const T& item) {
      auto [itr, inserted] = storages.try_emplace(FNV(stream_name), stream_storage{ sizeof(T), 0, {} });
      stream_storage& storage = itr->second;
      if (!inserted) {
        if (storage.element_size != sizeof(T)) {
          CORE_LOG_ERROR("Debug stream '{}' already has element size {}, cannot submit item of size {}", stream_name, storage.element_size, sizeof(T));
          return;
        }
      }
      if (storage.count >= storage.bytes.size() / storage.element_size) {
        CORE_LOG_WARN("Debug stream '{}' has reached maximum capacity of {} elements, cannot submit more items", stream_name, storage.count);
        return;
      }
      const uint8_t* item_bytes = reinterpret_cast<const uint8_t*>(&item);
      storage.bytes.append_range(std::span<const uint8_t>(item_bytes, sizeof(T)));
      storage.count++;
    }

    void configure_stream(std::string_view stream_name, size_t elt_size, size_t max_per_frame);

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
    };
    std::map<natural_t, stream_storage> storages;

    stream_storage& ensure_storage(std::string_view name, size_t element_size);
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DEBUG_RENDER_STREAM_HPP