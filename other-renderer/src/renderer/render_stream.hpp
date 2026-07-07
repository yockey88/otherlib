/**
 * \file renderer/render_stream.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_RENDER_STREAM_HPP
#define OTHER_RENDERER_RENDERER_RENDER_STREAM_HPP

#include "renderer/render_stream_registry.hpp"

namespace other {

  class renderer;

  class render_stream {
   public:
    template <typename T>
    void submit(std::string_view stream_name, const T& item) {
      auto itr = storages.find(FNV(stream_name));
      if (itr == storages.end()) {
        CORE_LOG_ERROR("Stream with name [{}] not found. Cannot submit item.", stream_name);
        return;
      }

      const uint8_t* item_bytes = reinterpret_cast<const uint8_t*>(&item);
      itr->second.bytes.append_range(std::span<const uint8_t>(item_bytes, sizeof(T)));
      itr->second.count++;
    }

    void configure_streams(renderer* renderer_ptr, const render_stream_registry& registry);
    void configure_stream(renderer* renderer_ptr, const render_stream_definition& defn);

    resource_handle get_mesh_handle(std::string_view stream_name) const;

    std::span<const uint8_t> view(std::string_view stream_name) const;
    size_t count(std::string_view stream_name) const;
    size_t element_size(std::string_view stream_name) const;

    void clear();

   private:
    struct render_stream_storage {
      size_t element_size;
      size_t max_per_frame = 0;
      size_t count;
      ostd::vector<uint8_t> bytes;

      // render stream not responsible for managing this mesh resource
      // the renderer will create/clean up these resources
      resource_handle mesh_handle;
    };
    ostd::map<natural_t, render_stream_storage> storages;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_RENDER_STREAM_HPP