/**
 * \file renderer/debug_render_stream.cpp
 **/
#include "renderer/debug_render_stream.hpp"

#include "core/fnv.hpp"

#include "renderer/renderer.hpp"

namespace other {

  void debug_stream_registry::register_stream(std::string_view name, debug_stream_definition defn) {
    natural_t hash = FNV(name);
    auto itr = defs.find(hash);
    if (itr != defs.end()) {
      CORE_LOG_ERROR("Debug stream with name [{}] already exists. Ignoring registration.", name);
      return;
    }
    defn.name = std::string(name);
    defs.insert({ hash, std::move(defn) });
  }

  const debug_stream_definition* debug_stream_registry::find(std::string_view name) const {
    natural_t hash = FNV(name);
    auto itr = defs.find(hash);
    if (itr == defs.end()) {
      return nullptr;
    }
    return &itr->second;
  }

  void debug_streams::configure_streams(renderer* renderer_ptr, const debug_stream_registry& registry) {
    OTHER_ASSERT(renderer_ptr != nullptr, "debug_streams::configure_streams: renderer_ptr is null.");
    for (const auto& [_, def] : registry.entries()) {
      configure_stream(renderer_ptr, def);
    }
  }

  void debug_streams::configure_stream(renderer* renderer_ptr, const debug_stream_definition& defn) {
    OTHER_ASSERT(renderer_ptr != nullptr, "debug_streams::configure_stream: renderer_ptr is null.");
    natural_t hash = FNV(defn.name);
    auto [itr, _] = storages.try_emplace(hash, stream_storage{});
    OTHER_ASSERT(itr != storages.end(), "Failed to insert debug stream storage for stream '{}'", defn.name);

    itr->second.element_size = defn.element_size;
    itr->second.max_per_frame = defn.max_per_frame;
    itr->second.bytes.reserve(defn.element_size * defn.max_per_frame);
    itr->second.mesh_handle = renderer_ptr->get_or_create_debug_stream_mesh(defn.name, defn);
  }

  resource_handle debug_streams::get_mesh_handle(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    OTHER_ASSERT(itr != storages.end(), "Debug stream with name [{}] not found. Cannot get mesh handle.", stream_name);
    return itr->second.mesh_handle;
  }

  std::span<const uint8_t> debug_streams::view(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return {};
    }
    return itr->second.bytes;
  }

  size_t debug_streams::count(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return 0;
    }
    return itr->second.count;
  }

  size_t debug_streams::element_size(std::string_view stream_name) const {
    auto itr = storages.find(FNV(stream_name));
    if (itr == storages.end()) {
      return 0;
    }
    return itr->second.element_size;
  }

  void debug_streams::clear() {
    for (auto& [_, storage] : storages) {
      storage.bytes.clear();
      storage.count = 0;
    }
    storages.clear();
  }

}  // namespace other