/**
 * \file plugin/plugin_interface.hpp
 **/
#ifndef OTHER_PLUGIN_PLUGIN_INTERFACE_HPP
#define OTHER_PLUGIN_PLUGIN_INTERFACE_HPP

namespace other {

#define OTHER_PROVIDES(ImplT, InterfT, instance_name)                                                      \
  static_assert(std::derived_from<ImplT, InterfT>, "Implementation type must derive from interface type"); \
  extern "C" OTHER_API InterfT* __create_##ImplT(const config_table* config) {                             \
    return other::arena_allocator<ImplT>{}.allocate();                                                     \
  }                                                                                                        \
  namespace {                                                                                              \
    struct __other_registry_##ImplT {                                                                      \
      __other_registry_##ImplT() {                                                                         \
        other::detail::plugin_registry::add_entry({ other::detail::manifest_entry::{                       \
          .slot_type_hash = typeid(InterfT).hash_code(),                                                   \
          .factory_symbol = "__create_" #ImplT,                                                            \
          .instance_name = instance_name,                                                                  \
        } });                                                                                              \
      }                                                                                                    \
    };                                                                                                     \
    static __other_registry_##ImplT __other_registry_instance_##ImplT{};                                   \
  }

}  // namespace other

#endif  // OTHER_PLUGIN_PLUGIN_INTERFACE_HPP