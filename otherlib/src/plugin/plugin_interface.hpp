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
  constexpr inline std::string_view kMetadataFunctionSymbolName = "_other_plugin_get_plugin_metadata";

  struct plugin_metadata {
    const char* name;     // can really be anything
    const char* version;  // must be x.x.x
    const char* author;
    const char* description;
    /* license, homepage, dependencies, etc. */
  };

  namespace detail {

    constexpr plugin_param_view pick_params_view() { return plugin_param_view{}; }
    constexpr plugin_param_view pick_params_view(plugin_param_view v) { return v; }

  }  // namespace detail
}  // namespace other

#ifdef OTHER_DYNAMIC_DRIVER
  #define OTHER_PLUGIN_HANDSHAKE_BODY(name) ::other::plugin::on_enter(other_plugin_name(), argv);
  #define OTHER_ENVIRONMENT_DYN_DRIVER_FUNCTIONS(name)
#endif
#ifdef OTHER_PLUGIN_DYNAMIC_LIBRARY
  #define OTHER_PLUGIN_HANDSHAKE_BODY(name) ::other::plugin::on_enter(other_plugin_name(), argv);
  #define OTHER_ENVIRONMENT_DYN_DRIVER_FUNCTIONS(name)                                                                                            \
    extern "C" ::other::driver* otherlib_create_driver(const ::other::command_line* cmd, const ::other::config_table* config) { return nullptr; } \
    extern "C" void otherlib_destroy_driver(::other::driver* instance) { (void)instance; }
#endif
#ifdef OTHER_STATIC_DRIVER
  #define OTHER_PLUGIN_HANDSHAKE_BODY(name)
  #define OTHER_ENVIRONMENT_DYN_DRIVER_FUNCTIONS(name)
#endif

#define OTHER_PLUGIN(plugin_name, version_str, author_str, description_str)                  \
  extern "C" OTHER_API const char* other_plugin_name() { return #plugin_name; }              \
  extern "C" OTHER_API void bind_plugin_systems(::other::other_plugin_argv* argv) {          \
    OTHER_PLUGIN_HANDSHAKE_BODY(plugin_name)                                                 \
  }                                                                                          \
  extern "C" OTHER_API const ::other::plugin_metadata* _other_plugin_get_plugin_metadata() { \
    static ::other::plugin_metadata metadata{                                                \
      .name = #plugin_name,                                                                  \
      .version = version_str,                                                                \
      .author = author_str,                                                                  \
      .description = description_str                                                         \
    };                                                                                       \
    return &metadata;                                                                        \
  }                                                                                          \
  OTHER_ENVIRONMENT_DYN_DRIVER_FUNCTIONS(plugin_name)

#define _OTHER_PLUGIN_FACTORY_SYMBOL_NAME(ImplT) _create_##ImplT

#define OTHER_PROVIDES(ImplT, InterfT, inst_name, ...)                                                                                      \
  static_assert(::other::kIsEnvironmentInterface<InterfT>, "Interface type must satisfy environment_interface concept");                    \
  static_assert(::std::derived_from<ImplT, InterfT>, "Implementation type must derive from interface type");                                \
  static_assert(::other::kConstructibleFromTuple<ImplT, typename InterfT::construction_args_t>,                                             \
                "Implementation must be constructible from the interface's declared construction args. "                                    \
                "Check that " #ImplT "'s constructor signature matches the types declared in " #InterfT "'s OTHER_ENVIRONMENT_INTERFACE."); \
  extern "C" OTHER_API InterfT* _OTHER_PLUGIN_FACTORY_SYMBOL_NAME(ImplT)(void* args_blob) {                                                 \
    using args_t = typename InterfT::construction_args_t;                                                                                   \
    const args_t& args = *reinterpret_cast<args_t*>(args_blob);                                                                             \
    return std::apply([](auto&&... a) { return ::other::arena_allocator<ImplT>{}.allocate(std::forward<decltype(a)>(a)...); }, args);       \
  }                                                                                                                                         \
  extern "C" OTHER_API ::other::plugin_manifest* _other_plugin_get_plugin_manifest() {                                                      \
    static ::other::plugin_manifest manifest{                                                                                               \
      .interface_hash = InterfT::kInterfaceHash,                                                                                            \
      .factory_function = reinterpret_cast<void* (*)(void*)>(_OTHER_PLUGIN_FACTORY_SYMBOL_NAME(ImplT)),                                     \
      .class_name = #ImplT,                                                                                                                 \
      .plugin_instance_name = inst_name,                                                                                                    \
      .parameters = ::other::detail::pick_params_view(__VA_OPT__(__VA_ARGS__)),                                                             \
    };                                                                                                                                      \
    return &manifest;                                                                                                                       \
  }

#endif  // OTHER_PLUGIN_PLUGIN_INTERFACE_HPP