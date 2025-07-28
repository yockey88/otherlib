/**
 * \file scene/scene.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_HPP
#define OTHER_SCENE_SCENE_SCENE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"

#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "renderer/renderer.hpp"

#include "scene/scene_tree.hpp"

namespace other {

  class scene {
   public:
    scene();
    ~scene();

    scene_object& root_object();

    scene_object& create_object(const std::string& name, scene_object* parent_object = nullptr);
    scene_object& create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object = nullptr);

    void destroy_object(natural_t id);

    scene_object& get_object(natural_t id);

    size_t get_object_count() const;

    transform& get_transform(scene_object* object);
    const transform& get_transform(const scene_object* object) const;
    void set_transform(scene_object* object, const transform& t);

    glm::mat4 get_world_transform(scene_object* obj) const;
    glm::mat4 get_world_transform(natural_t id) const;

    transform& get_transform(natural_t id);
    const transform& get_transform(natural_t id) const;
    void set_transform(natural_t id, const transform& t);

    render_data prepare_render_data() const;

    bool object_has_tag(natural_t id, const std::string_view tag) const;

    void add_object_tag(natural_t id, const std::string_view tag);

    template <typename T>
    T& add_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot add component to a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return registry.emplace<T>(entity);
    }
    template <typename T>
    T& add_component(natural_t id) {
      scene_tree::node* node = tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
      return add_component<T>(node->object);
    }

    template <typename T>
    T* get_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot get component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return registry.try_get<T>(entity);
    }
    template <typename T>
    T* get_component(natural_t id) {
      scene_tree::node* node = tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
      return get_component<T>(node->object);
    }

    template <typename T>
    const T* get_component(const scene_object* object) const {
      OTHER_ASSERT(object != nullptr, "Cannot get component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return registry.try_get<T>(entity);
    }
    template <typename T>
    const T* get_component(natural_t id) const {
      const scene_tree::node* node = tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
      return get_component<T>(node->object);
    }

    template <typename T>
    void remove_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot remove component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      registry.remove<T>(entity);
    }
    template <typename T>
    void remove_component(natural_t id) {
      scene_tree::node* node = tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
      remove_component<T>(node->object);
    }

    template <typename T>
    bool has_component(scene_object* object) const {
      OTHER_ASSERT(object != nullptr, "Cannot check component on a null scene object.");
      return get_component<T>(object) != nullptr;
    }
    template <typename T>
    bool has_component(natural_t id) const {
      const scene_tree::node* node = tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
      return has_component<T>(node->object);
    }

    static std::string as_string(const scene& s);

   private:
    struct object_handle {
      natural_t id = 0;
      scene_object* object = nullptr;

      operator scene_object*() const;
      bool operator==(const object_handle& other) const;
    };
    friend class scene_tree;

    void register_object(scene_object* object, const std::string& name, const glm::vec3& world_position);
    void unregister_object(scene_object* object);

    void on_create_render_component(const entt::registry&, const entt::entity entity);
    void on_update_render_component(const entt::registry&, const entt::entity entity);
    void on_destroy_render_component(const entt::registry&, const entt::entity entity);

    void on_create_script_component(const entt::registry&, const entt::entity entity);
    // void on_update_script_component(const entt::registry&, const entt::entity entity);
    void on_destroy_script_component(const entt::registry&, const entt::entity entity);

    entt::registry registry;
    scene_tree tree;

    std::optional<render_data> render_data_cache = std::nullopt;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_HPP