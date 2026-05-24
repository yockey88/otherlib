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

  template <typename T>
  void debug_streams::submit(std::string_view stream_name, const T& item) {
    auto [itr, inserted] = storages.try_emplace(FNV(stream_name), stream_storage{ sizeof(T), 0, {} });
    stream_storage& storage = itr->second;
    if (!inserted) {
      if (storage.element_size != sizeof(T)) {
        CORE_LOG_ERROR("Debug stream '{}' already has element size {}, cannot submit item of size {}", stream_name, storage.element_size, sizeof(T));
        return;
      }
    }
    if (storage.count >= storage.bytes.size() / storage.element_size) {
      CORE_LOG_WARNING("Debug stream '{}' has reached maximum capacity of {} elements, cannot submit more items", stream_name, storage.count);
      return;
    }
    const uint8_t* item_bytes = reinterpret_cast<const uint8_t*>(&item);
    storage.bytes.append_range(std::span<const uint8_t>(item_bytes, sizeof(T)));
    storage.count++;
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

}  // namespace other