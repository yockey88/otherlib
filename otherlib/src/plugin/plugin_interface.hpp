/**
 * \file plugin/plugin_interface.hpp
 **/
#ifndef OTHER_PLUGIN_PLUGIN_INTERFACE_HPP
#define OTHER_PLUGIN_PLUGIN_INTERFACE_HPP

#include "core/build_config.hpp"
#include "core/command_line.hpp"
#include "core/config_table.hpp"
#include "core/interfaces.hpp"

#include "plugin/plugin_manifest.hpp"

namespace other {

  constexpr inline std::string_view kManifestFunctionSymbolName = "_other_plugin_get_plugin_manifest";

}  // namespace other

#ifdef OTHER_DYNAMIC_DRIVER
  #define OTHER_PLUGIN(name)                                                                                                       \
    extern "C" {                                                                                                                   \
    OTHER_API const char* other_plugin_name() { return #name; }                                                                    \
    OTHER_API void bind_plugin_systems(::other::other_plugin_argv* argv) { ::other::plugin::on_enter(other_plugin_name(), argv); } \
    }
#endif

#ifdef OTHER_PLUGIN_DYNAMIC_LIBRARY
  #define OTHER_PLUGIN(name)                                                                                                           \
    extern "C" {                                                                                                                       \
    OTHER_API const char* other_plugin_name() { return #name; }                                                                        \
    OTHER_API void bind_plugin_systems(::other::other_plugin_argv* argv) { ::other::plugin::on_enter(other_plugin_name(), argv); }     \
    }                                                                                                                                  \
    ::other::driver* otherlib_create_driver(const ::other::command_line* cmd, const ::other::config_table* config) { return nullptr; } \
    void otherlib_destroy_driver(::other::driver* instance) {}
#endif

#ifdef OTHER_STATIC_DRIVER
  #define OTHER_PLUGIN(name)                                                \
    extern "C" {                                                            \
    OTHER_API const char* other_plugin_name() { return nullptr; }           \
    OTHER_API void bind_plugin_systems(::other::other_plugin_argv* argv) {} \
    }
#endif

#define OTHER_PROVIDES(ImplT, InterfT, inst_name)                                                                        \
  static_assert(::other::kIsEnvironmentInterface<InterfT>, "Interface type must satisfy environment_interface concept"); \
  static_assert(::std::derived_from<ImplT, InterfT>, "Implementation type must derive from interface type");             \
  extern "C" OTHER_API inline InterfT* __create_##ImplT() {                                                              \
    return ::other::arena_allocator<ImplT>{}.allocate();                                                                 \
  }                                                                                                                      \
  extern "C" OTHER_API inline ::other::plugin_manifest* _other_plugin_get_plugin_manifest() {                            \
    static ::other::plugin_manifest manifest{                                                                            \
      .interface_hash = InterfT::kInterfaceHash,                                                                         \
      .factory_function = reinterpret_cast<void* (*)()>(__create_##ImplT),                                               \
      .class_name = #ImplT,                                                                                              \
      .plugin_instance_name = inst_name                                                                                  \
    };                                                                                                                   \
    return &manifest;                                                                                                    \
  }

#endif  // OTHER_PLUGIN_PLUGIN_INTERFACE_HPP