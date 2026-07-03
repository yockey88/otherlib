/**
 * \file renderer/render_stream.cpp
 **/
#include "renderer/render_stream.hpp"

#include "renderer/renderer.hpp"

namespace other {
  void render_stream::configure_streams(renderer* renderer_ptr, const render_stream_registry& registry) {
    OTHER_ASSERT(renderer_ptr != nullptr, "render_stream::configure_streams: renderer_ptr is null.");
    for (const auto& [_, def] : registry.entries()) {
      configure_stream(renderer_ptr, def);
    }
  }

  void render_stream::configure_stream(renderer* renderer_ptr, const render_stream_definition& defn) {
    OTHER_ASSERT(renderer_ptr != nullptr, "render_stream::configure_stream: renderer_ptr is null.");
    natural_t hash = FNV(defn.name);
    auto [itr, success] = storages.try_emplace(hash, render_stream_storage{});
    if (!success) {
      bool coherent = true;
      if (defn.element_size != itr->second.element_size) {
        CORE_LOG_ERROR("Stream [{}] already exists, element size mismatch (existing: {}, new: {}).", defn.name, itr->second.element_size, defn.element_size);
        coherent = false;
      }
      if (defn.max_per_frame != itr->second.max_per_frame) {
        CORE_LOG_ERROR("Stream [{}] already exists, max per frame mismatch (existing: {}, new: {}).", defn.name, itr->second.max_per_frame, defn.max_per_frame);
        coherent = false;
      }
      if (!coherent) {
        CORE_LOG_ERROR("Stream [{}] already exists. Cannot configure stream.", defn.name);
      }
    } else {
      itr->second.element_size = defn.element_size;
      itr->second.max_per_frame = defn.max_per_frame;
      itr->second.bytes.reserve(defn.element_size * defn.max_per_frame);
      itr->second.mesh_handle = renderer_ptr->get_or_create_stream_mesh(defn.name, defn);
    }
  }

  resource_handle render_stream::get_mesh_handle(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    OTHER_ASSERT(itr != storages.end(), "Stream with name [{}] not found. Cannot get mesh handle.", stream_name);
    return itr->second.mesh_handle;
  }

  std::span<const uint8_t> render_stream::view(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return {};
    }
    return itr->second.bytes;
  }

  size_t render_stream::count(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return 0;
    }
    return itr->second.count;
  }

  size_t render_stream::element_size(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return 0;
    }
    return itr->second.element_size;
  }

  void render_stream::clear() {
    for (auto& [_, storage] : storages) {
      storage.bytes.clear();
      storage.count = 0;
    }
    storages.clear();
  }

}  // namespace other