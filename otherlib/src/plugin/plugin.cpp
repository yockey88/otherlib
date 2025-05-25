/**
 * \file plugin/plugin.cpp
 **/
#include "plugin/plugin.hpp"

#include "core/arena.hpp"
#include "core/fnv.hpp"
#include "core/logger.hpp"

namespace other {

  std::map<uint64_t, library_handle*> plugin::loaded_libraries;

  library_handle* plugin::load_plugin_library(const std::string_view plugin_path) {
    if (plugin_path.empty()) {
      CORE_LOG_ERROR("Plugin path is empty");
      return nullptr;
    } else if (!std::filesystem::exists(plugin_path)) {
      CORE_LOG_ERROR("Plugin path does not exist: {}", plugin_path);
      return nullptr;
    }

    std::string name = filepath(plugin_path).filename().stem().string();

    uint64_t hash = FNV(name);
    auto it = loaded_libraries.find(hash);
    if (it != loaded_libraries.end()) {
      return it->second;
    }

    library_handle* lib_handle = create_library_handle(plugin_path);
    if (lib_handle == nullptr) {
      CORE_LOG_ERROR("Failed to create library handle for plugin '{}'", plugin_path);
      return nullptr;
    }
    lib_handle->load();
    if (!lib_handle->is_loaded()) {
      CORE_LOG_ERROR("Failed to load plugin library '{}'", plugin_path);
      return nullptr;
    }

    symbol& sym = lib_handle->get_symbol("bind_plugin_systems");
    if (sym.address == nullptr) {
      CORE_LOG_ERROR("Failed to load symbol '{}' from plugin '{}'", plugin::kPluginBindingSymbolName, plugin_path);
      return nullptr;
    }
    other_plugin_argv argv = {
      subsystem<arena>::get(),
      subsystem<logger>::get(),
    };
    sym.get_function<void (*)(other_plugin_argv*)>()(&argv);

    auto [itr2, success] = loaded_libraries.insert({ hash, std::move(lib_handle) });
    if (!success || itr2 == loaded_libraries.end()) {
      CORE_LOG_ERROR("Failed to insert library handle into map for plugin '{}'", plugin_path);
      throw std::runtime_error("Failed to insert library handle into map");
    }

    return itr2->second;
  }

  library_handle* plugin::get_plugin_library(const std::string_view plugin_name) {
    if (plugin_name.empty()) {
      CORE_LOG_ERROR("Plugin name is empty");
      return nullptr;
    }

    uint64_t hash = FNV(plugin_name);
    auto it = loaded_libraries.find(hash);
    if (it != loaded_libraries.end()) {
      return it->second;
    }

    CORE_LOG_ERROR("Plugin library '{}' not found", plugin_name);
    return nullptr;
  }

  void plugin::unload_plugin_library(const std::string_view plugin_path) {
    for (auto& [hash, lib_handle] : loaded_libraries) {
      if (lib_handle != nullptr) {
        lib_handle->unload();
      }
    }
    loaded_libraries.clear();
  }

  void plugin::unload_all_plugin_libraries() {
    for (auto& [hash, lib_handle] : loaded_libraries) {
      if (lib_handle != nullptr) {
        lib_handle->unload();
      }
    }
    loaded_libraries.clear();
  }

}  // namespace other