/**
 * \file plugin/plugin.hpp
 **/
#ifndef OTHER_PLUGIN_PLUGIN_HPP
#define OTHER_PLUGIN_PLUGIN_HPP

#include "core/defines.hpp"

#include "plugin/library_handle.hpp"

namespace other {

  class arena;
  class logger;
  struct OTHER_CLASS other_plugin_argv {
    arena* arena = nullptr;
    logger* logger = nullptr;
  };

  class plugin {
   public:
    static library_handle* load_plugin_library(const std::string_view plugin_path);
    static library_handle* get_plugin_library(const std::string_view plugin_name);

    static void unload_plugin_library(const std::string_view plugin_path);
    static void unload_all_plugin_libraries();

   private:
    constexpr static const char* kPluginBindingSymbolName = "bind_plugin_systems";
    static std::map<uint64_t, library_handle*> loaded_libraries;

    static library_handle* create_library_handle(const std::string_view plugin_path);
  };

#define OTHER_PLUGIN(name)                                             \
  OTHER_API const char* other_plugin_name() { return #name; }          \
  OTHER_API void bind_plugin_systems(other::other_plugin_argv* argv) { \
    other::subsystem<other::arena>::set(argv->arena);                  \
    other::subsystem<other::logger>::set(argv->logger);                \
  }

}  // namespace other

#endif  // OTHER_PLUGIN_PLUGIN_HPP