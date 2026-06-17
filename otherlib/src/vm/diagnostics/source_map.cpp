/**
 * \file vm/diagnostics/source_map.cpp
 **/
#include "vm/diagnostics/source_map.hpp"

#include "core/fnv.hpp"

namespace other {

  natural_t source_map::add_source(const std::string_view name, const std::string_view text) {
    natural_t hash = FNV(name);
    auto [itr, inserted] = sources.emplace(hash, source{ std::string{ name }, std::string(text) });
    if (!inserted) {
      CORE_LOG_ERROR("Failed to insert source: {}", name);
      return 0;
    }
    return hash;
  }

  std::string_view source_map::get_source_text(natural_t id) {
    auto itr = sources.find(id);
    if (itr == sources.end()) {
      CORE_LOG_ERROR("Source not found: {}", id);
      return {};
    }
    return itr->second.text;
  }

  std::string_view source_map::get_source_name(natural_t id) {
    auto itr = sources.find(id);
    if (itr == sources.end()) {
      CORE_LOG_ERROR("Source not found: {}", id);
      return {};
    }
    return itr->second.name;
  }

}  // namespace other