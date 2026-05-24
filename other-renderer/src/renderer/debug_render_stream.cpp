/**
 * \file renderer/debug_render_stream.cpp
 **/
#include "renderer/debug_render_stream.hpp"

#include "core/fnv.hpp"

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

  void debug_streams::configure_stream(std::string_view stream_name, size_t elt_size, size_t max_per_frame) {
    natural_t hash = FNV(stream_name);
    auto [itr, _] = storages.try_emplace(hash, stream_storage{});
    itr->second.element_size = elt_size;
    itr->second.max_per_frame = max_per_frame;
    itr->second.bytes.reserve(elt_size * max_per_frame);
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
  }

  debug_streams::stream_storage& debug_streams::ensure_storage(std::string_view name, size_t element_size) {
    natural_t hash = FNV(name);
    auto [itr, inserted] = storages.try_emplace(hash, stream_storage{
                                                        .element_size = element_size,
                                                        .max_per_frame = 0,  // populated by configure_stream
                                                        .count = 0,
                                                        .bytes = {},
                                                      });
    if (inserted) {
      itr->second.bytes.reserve(element_size * 64);
    }
    return itr->second;
  }

}  // namespace other