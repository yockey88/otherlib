/**
 * \file plugin/library_handle.cpp
 **/
#include "plugin/library_handle.hpp"

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "plugin/plugin.hpp"

namespace other {

  symbol& library_handle::get_symbol(const std::string_view sym) {
    uint64_t hash = FNV(sym);
    auto it = symbols.find(hash);
    if (it != symbols.end()) {
      return it->second;
    }

    symbol sym_obj = load_symbol(sym);
    auto [it2, inserted] = symbols.insert({ hash, sym_obj });
    if (!inserted) {
      throw std::runtime_error(std::format("Failed to insert symbol into map : {}", sym));
    }

    return it2->second;
  }

  void library_handle::call_plugin_binder(other_plugin_argv* argv) {
    subsystem<arena>::set(argv->arena);
    subsystem<logger>::set(argv->logger);
  }

}  // namespace other