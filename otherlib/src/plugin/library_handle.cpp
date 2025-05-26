/**
 * \file plugin/library_handle.cpp
 **/
#include "plugin/library_handle.hpp"

#include "core/fnv.hpp"

namespace other {

  std::expected<symbol, std::nullptr_t> library_handle::get_symbol(const std::string_view sym) {
    uint64_t hash = FNV(sym);
    auto it = symbols.find(hash);
    if (it != symbols.end()) {
      return it->second;
    }

    symbol sym_obj = load_symbol(sym);
    auto [it2, inserted] = symbols.insert({ hash, sym_obj });
    if (!inserted || it2 == symbols.end()) {
      return std::unexpected(nullptr);
    } else if (it2->second.address == nullptr) {
      return std::unexpected(nullptr);
    }

    return it2->second;
  }

}  // namespace other