/**
 * \file object/object_component_registry.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_OBJECT_COMPONENT_REGISTRY_HPP
#define OTHER_SCENE_OBJECT_OBJECT_COMPONENT_REGISTRY_HPP

#include <map>
#include <set>

#include "core/defines.hpp"

namespace other {

  struct object_component_registry {
    struct component_data {
      natural_t type_hash = 0;
      void* comp_ptr = nullptr;
    };
    std::set<size_t> registered_component_types;
    ostd::unordered_map<size_t, component_data> components;

    template <typename T>
    void register_component(T& comp) {
      if (registered_component_types.find(typeid(T).hash_code()) != registered_component_types.end()) {
        CORE_LOG_WARN("Component of type '{}' is already registered in the component registry.", typeid(T).name());
        return;
      }
      auto [itr, succ] = components.emplace(typeid(T).hash_code(), component_data{ .type_hash = typeid(T).hash_code(), .comp_ptr = &comp });
      OTHER_ASSERT(succ, "Component of type '{}' is already registered in the component registry.", typeid(T).name());
      registered_component_types.insert(typeid(T).hash_code());
    }

    template <typename T>
    void remove_component() {
      if (!registered_component_types.contains(typeid(T).hash_code())) {
        CORE_LOG_WARN("Component of type '{}' is not registered in the component registry.", typeid(T).name());
        return;
      }
      registered_component_types.erase(typeid(T).hash_code());
      components.erase(typeid(T).hash_code());
    }
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP