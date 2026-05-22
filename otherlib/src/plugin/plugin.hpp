/**
 * \file plugin/plugin.hpp
 **/
#ifndef OTHERLIB_PLUGIN_PLUGIN_HPP
#define OTHERLIB_PLUGIN_PLUGIN_HPP

#include <mutex>

#include <imgui/imgui.h>

#include "core/defines.hpp"

#include "plugin/library_handle.hpp"
#include "plugin/plugin_interface.hpp"

#define XSTRINGIFY(a) STRINGIFY(a)
#define STRINGIFY(a) #a

namespace other {

  class arena;
  class logger;
  class file_system;
  class input_system;
  class type_database;
  class physics_environment;
  class renderer_backend;
  class scripting_environment;
  class driver;
  struct OTHER_CLASS other_plugin_argv {
    arena* arena = nullptr;
    logger* logger = nullptr;
    file_system* file_system = nullptr;
    input_system* input_system = nullptr;
    type_database* type_database = nullptr;
    physics_environment* physics_environment = nullptr;
    renderer_backend* renderer = nullptr;
    scripting_environment* scripting_environment = nullptr;
  };

  class plugin {
   public:
    static library_handle* load_plugin_library(const std::string_view plugin_path);
    static library_handle* get_plugin_library(const std::string_view plugin_name);

    static void unload_plugin_library(const std::string_view plugin_name);

    /// FROM PLUGIN SIDE ONLY:
    static void on_enter(const std::string_view pl_name, other_plugin_argv* argv);

   private:
    constexpr static const char* kPluginBindingSymbolName = "bind_plugin_systems";
    static std::mutex plugin_mutex;
    static std::map<natural_t, library_handle*> loaded_libraries;

    static library_handle* create_library_handle(const std::string_view plugin_path);

    static void set_subsystem_flags(const std::string_view pl_name, other_plugin_argv* argv);
    static void plugin_binding(const std::string_view pl_name, other_plugin_argv* argv);
  };

}  // namespace other

#endif  // OTHERLIB_PLUGIN_PLUGIN_HPP