/**
 * \file renderer/render_stream_registry.cpp
 **/
#include "renderer/render_stream_registry.hpp"

#include "core/fnv.hpp"

#include "renderer/renderer.hpp"

namespace other {

  void render_stream_registry::register_stream(std::string_view name, render_stream_definition defn) {
    natural_t hash = FNV(name);
    auto itr = defs.find(hash);
    if (itr != defs.end()) {
      CORE_LOG_ERROR("Stream with name [{}] already exists. Ignoring registration.", name);
      return;
    }
    defn.name = std::string(name);
    defs.insert({ hash, std::move(defn) });
  }

  const render_stream_definition* render_stream_registry::find(std::string_view name) const {
    natural_t hash = FNV(name);
    auto itr = defs.find(hash);
    if (itr == defs.end()) {
      return nullptr;
    }
    return &itr->second;
  }

}  // namespace other