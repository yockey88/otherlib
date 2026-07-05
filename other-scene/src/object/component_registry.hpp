/**
 * \file object/component_registery.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP
#define OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP

#include <utility>

#include "scene/scene.hpp"

namespace other {

  using scene_object_has_component_fn = std::function<bool(scene*, scene_object*)>;
  using scene_object_component_fn = std::function<void(scene*, scene_object*)>;
  /// can we do this without std::any? possibly using value?
  using component_snapshot_fn = std::function<std::any(scene*, scene_object*)>;
  using component_restore_fn = std::function<void(scene*, scene_object*, const std::any&)>;

  struct component_type_flags {
    bool removable = true;
    bool implicit = false;
  };

  struct component_registration {
    std::string component_name;
    natural_t type_id = 0;
    glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
    component_type_flags flags{};

    scene_object_has_component_fn has_component = nullptr;
    scene_object_component_fn add_component = nullptr;
    scene_object_component_fn remove_component = nullptr;
    component_snapshot_fn snapshot_component = nullptr;
    component_restore_fn restore_component = nullptr;
  };

  class component_registry {
   public:
    component_registry() = default;
    ~component_registry() = default;

    template <typename T>
    natural_t register_component_type(const std::string_view name) {
      if (registry.contains(typeid(T).hash_code())) {
        CORE_LOG_WARN("Component type '{}' is already registered in the component registry.", name);
        return typeid(T).hash_code();
      }

      natural_t type_id = static_cast<natural_t>(typeid(T).hash_code());
      registry.emplace(type_id, component_registration{
                                  .component_name = std::string(name),
                                  .has_component = [n = std::string(name)](scene* s, scene_object* object) -> bool {
                                    OTHER_ASSERT(s != nullptr, "Scene pointer is null in component registry has_component check for component '{}'.", n);
                                    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in component registry has_component check for component '{}'.", n);
                                    return s->template has_component<T>(object);
                                  },
                                  .add_component = [n = std::string(name)](scene* s, scene_object* object) -> void {
                                    OTHER_ASSERT(s != nullptr, "Scene pointer is null in component registry add_component check for component '{}'.", n);
                                    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in component registry add_component check for component '{}'.", n);
                                    s->template add_component<T>(object);
                                  },
                                  .remove_component = [n = std::string(name)](scene* s, scene_object* object) -> void {
                                    OTHER_ASSERT(s != nullptr, "Scene pointer is null in component registry remove_component check for component '{}'.", n);
                                    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in component registry remove_component check for component '{}'.", n);
                                    s->template remove_component<T>(object);
                                  },
                                  .snapshot_component = [n = std::string(name)](scene* s, scene_object* object) -> std::any {
                                    OTHER_ASSERT(s != nullptr, "Scene pointer is null in component registry snapshot_component for component '{}'.", n);
                                    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in component registry snapshot_component for component '{}'.", n);
                                    if (!s->template has_component<T>(object)) {
                                      CORE_LOG_ERROR("Cannot snapshot component '{}' on object with ID {} because the object does not have that component.", n, object->id);
                                      return {};
                                    }
                                    return s->template get_component<T>(object);
                                  },
                                  .restore_component = [n = std::string(name)](scene* s, scene_object* object, const std::any& snapshot) -> void {
                                    OTHER_ASSERT(s != nullptr, "Scene pointer is null in component registry restore_component for component '{}'.", n);
                                    OTHER_ASSERT(object != nullptr, "Scene object pointer is null in component registry restore_component for component '{}'.", n);
                                    if (!snapshot.has_value()) {
                                      CORE_LOG_ERROR("Cannot restore component '{}' on object with ID {} because the snapshot does not have a value.", n, object->id);
                                      return;
                                    }
                                    if (snapshot.type() != typeid(T)) {
                                      CORE_LOG_ERROR("Cannot restore component '{}' on object with ID {} because the snapshot type does not match the component type.", n, object->id);
                                      return;
                                    }
                                    if (!s->template has_component<T>(object)) {
                                      CORE_LOG_ERROR("Cannot restore component '{}' on object with ID {} because the object does not have that component.", n, object->id);
                                      return;
                                    }
                                    *s->template get_component<T>(object) = std::any_cast<T>(snapshot);
                                  },
                                });
      return type_id;
    }

    template <typename T>
    component_registration get_registration() const {
      return get_registration(static_cast<natural_t>(typeid(T).hash_code()));
    }
    template <typename T>
    bool has_registration() const {
      return has_registration(static_cast<natural_t>(typeid(T).hash_code()));
    }

    template <typename T>
    bool has_component(scene* s, scene_object* object) const {
      auto reg = get_registration<T>();
      return reg.has_component(s, object);
    }
    template <typename T>
    void add_component(scene* s, scene_object* object) const {
      auto reg = get_registration<T>();
      reg.add_component(s, object);
    }
    template <typename T>
    void remove_component(scene* s, scene_object* object) const {
      auto reg = get_registration<T>();
      reg.remove_component(s, object);
    }
    template <typename T>
    std::any snapshot_component(scene* s, scene_object* object) const {
      auto reg = get_registration<T>();
      return reg.snapshot_component(s, object);
    }
    template <typename T>
    void restore_component(scene* s, scene_object* object, const std::any& snapshot) const {
      auto reg = get_registration<T>();
      reg.restore_component(s, object, snapshot);
    }

    component_registration get_registration(natural_t type_id) const;
    bool has_registration(natural_t type_id) const;

    inline const std::map<natural_t, component_registration>& get_registry() const { return registry; }

   private:
    std::map<natural_t, component_registration> registry;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_COMPONENT_REGISTRY_HPP