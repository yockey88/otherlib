/**
 * \file scene/scene.hpp
 **/
#ifndef OTHER_SCENE_SCENE_SCENE_HPP
#define OTHER_SCENE_SCENE_SCENE_HPP

#include <entt/entt.hpp>

#include "core/defines.hpp"

#include "renderer/debug_draw.hpp"
#include "renderer/renderer.hpp"

#include "object/component.hpp"
#include "object/component_registry.hpp"
#include "object/object_serialization_data.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene_storage.hpp"
#include "scene/scene_tree.hpp"

#include "asset/asset_handler.hpp"

namespace other {

  struct udp_handle;

  class scene {
   private:
    void scene_first_construction_initialization();
    void do_final_scene_destruction_cleanup();

    void do_scene_binding();
    void do_scene_unbinding();

   public:
    scene();
    scene(const std::string_view name);

    scene(scene&& other);
    scene& operator=(scene&& other);

    scene(const scene&) = delete;
    scene& operator=(const scene&) = delete;

    ~scene();

    void run_script_file();

    inline scene_storage& get_storage() {
      OTHER_ASSERT(storage != nullptr, "Scene storage is not initialized.");
      return *storage;
    }

    static scene create_scene(const std::string& name);

    void play();
    void pause();
    void stop();
    void reset();
    bool is_playing() const {
      return playing;
    }

    void enable_physics_debug_rendering();
    void disable_physics_debug_rendering();

    /// fixed update called at a constant timestep (e.g., 60 Hz)
    void fixed_update(double delta_time);

    /// per frame updates called with variable timestep
    void update(double delta_time);
    void late_update(double delta_time);

    scene_object& root_object();

    scene_object& create_object(const std::string& name, scene_object* parent_object = nullptr);
    scene_object& create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object = nullptr);

    scene_object& add_object(scene_object* object, const transform& transformation, scene_object* parent_object = nullptr);
    void add_objects(const std::span<serialization::parsed_scene_object> objects);

    scene_object* get_parent(natural_t id);
    const scene_object* get_parent(natural_t id) const;

    scene_object* get_parent(scene_object* object);
    const scene_object* get_parent(const scene_object* object) const;

    std::vector<uint64_t> get_children_ids(natural_t id) const;
    std::vector<uint64_t> get_children_ids(const scene_object* object) const;
    std::vector<scene_object*> get_children(natural_t id);
    std::vector<scene_object*> get_children(const scene_object* object);

    scene_object* find_object_with_tag(const std::string_view tag) const;

    std::vector<uint64_t> get_all_object_ids() const;

    void destroy_object(natural_t id);

    bool has_object(const std::string_view name) const;
    bool has_object(natural_t id) const;

    scene_object& get_object(const std::string_view name);
    const scene_object& get_object(const std::string_view name) const;

    scene_object& get_object(natural_t id);
    const scene_object& get_object(natural_t id) const;

    scene_object* find_object(const std::string_view name) const;
    scene_object* find_object(natural_t id) const;

    size_t get_object_count() const;

    transform& get_transform(scene_object* object);
    const transform& get_transform(const scene_object* object) const;
    void set_transform(scene_object* object, const transform& t);

    glm::mat4 get_world_transform(scene_object* obj) const;
    glm::mat4 get_world_transform(natural_t id) const;

    glm::mat4 get_local_transform(scene_object* obj) const;
    glm::mat4 get_local_transform(natural_t id) const;

    glm::mat4 get_local_to_world_matrix(scene_object* obj) const;
    glm::mat4 get_local_to_world_matrix(natural_t id) const;

    transform& get_transform(natural_t id);
    const transform& get_transform(natural_t id) const;
    void set_transform(natural_t id, const transform& t);

    bounding_box get_bounding_box(scene_object* object) const;
    bounding_box get_bounding_box(natural_t id) const;

    bounding_box get_bounding_box(std::span<scene_object*> objects) const;
    bounding_box get_bounding_box(std::span<const natural_t> ids) const;

    bounding_box get_bounding_box() const;
    bounding_box get_bounding_box_from_camera_frustum(const camera& cam) const;

    render_data prepare_render_data(const glm::ivec2 window_size, scope<asset_handler>& asset_handler) const;
    void debug_render(debug_draw draw);

    bool object_has_tag(natural_t id, const std::string_view tag) const;
    void add_object_tag(natural_t id, const std::string_view tag);
    void remove_object_tag(natural_t id, const std::string_view tag);

    inline lua_sandbox& get_sandbox() {
      OTHER_ASSERT(storage != nullptr, "Scene storage is null.");
      return storage->sandbox;
    }

    template <typename T>
      requires std::is_base_of_v<component, T>
    void register_component(scene_object* object, T& comp) {
      OTHER_ASSERT(object != nullptr, "Cannot register component to a null scene object.");

      auto* comp_reg = get_component<component_registry>(object);
      OTHER_ASSERT(comp_reg != nullptr, "Component registry not found for object with ID {}", object->id);

      comp_reg->register_component(comp);
    }

    template <typename T>
      requires std::is_base_of_v<component, T>
    T& add_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot add component to a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      auto& comp = storage->registry.emplace<T>(entity);
      register_component<T>(object, comp);
      return comp;
    }

    template <typename T>
    T& add_component(natural_t id) {
      scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      return add_component<T>(node->object);
    }

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    T& add_component(scene_object* object, Args&&... args) {
      OTHER_ASSERT(object != nullptr, "Cannot add component to a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      auto& comp = storage->registry.emplace<T>(entity, std::forward<Args>(args)...);
      register_component<T>(object, comp);
      return comp;
    }

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    T& add_component(natural_t id, Args&&... args) {
      scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      return add_component<T>(node->object, std::forward<Args>(args)...);
    }

    template <typename T>
    T* try_get_component(natural_t id) {
      scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      if (has_component<T>(node->object)) {
        return get_component<T>(node->object);
      } else {
        return nullptr;
      }
    }

    template <typename T>
    T* get_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot get component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return storage->registry.try_get<T>(entity);
    }

    template <typename T>
    T* get_component(natural_t id) {
      scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      return get_component<T>(node->object);
    }

    template <typename T>
    const T* get_component(const scene_object* object) const {
      OTHER_ASSERT(object != nullptr, "Cannot get component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);
      return storage->registry.try_get<T>(entity);
    }

    template <typename T>
    const T* get_component(natural_t id) const {
      const scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      return get_component<T>(node->object);
    }

    template <typename T>
    void remove_component(scene_object* object) {
      OTHER_ASSERT(object != nullptr, "Cannot remove component from a null scene object.");
      entt::entity entity = entt::entity(object->registry_id);

      auto* comp_reg = get_component<component_registry>(object);
      OTHER_ASSERT(comp_reg != nullptr, "Component registry not found for object with ID {}", object->id);

      comp_reg->remove_component<T>();
      comp_reg = nullptr;

      storage->registry.remove<T>(entity);
    }

    template <typename T>
    void remove_component(natural_t id) {
      scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      remove_component<T>(node->object);
    }

    template <typename T>
    bool has_component(scene_object* object) const {
      OTHER_ASSERT(object != nullptr, "Cannot check component on a null scene object.");
      return get_component<T>(object) != nullptr;
    }
    template <typename T>
    bool has_component(natural_t id) const {
      const scene_tree::node* node = storage->tree.node_at(id);
      OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
      return has_component<T>(node->object);
    }

    inline size_t get_num_objects() const {
      return storage != nullptr ? storage->tree.num_objects : 0;
    }

    static std::string as_string(const scene& s);

    void connect_remote_session(integer_t session_id);

    std::string name = "Untitled Scene";
    natural_t id = 0;
    natural_t asset_id = 0;

    /// clean this up, right now this is really fragile
    integer_t kNoStreamBinding = -1;
    integer_t update_stream_id = kNoStreamBinding;

    opt<filepath> script_path = std::nullopt;
    bool script_loaded = false;

    bool activate_on_load = false;
    bool synchronized = true;
    bool debug_physics_rendering_enabled = false;

   private:
    friend class scene_tree;
    struct object_handle {
      natural_t id = 0;
      scene_object* object = nullptr;

      operator scene_object*() const;
      bool operator==(const object_handle& other) const;
    };
    struct synchronization_state {
    };

    scene_object* from_registry_id(entt::entity entity);

    void register_object(scene_object* object, const std::string& name, const glm::vec3& world_position);
    void register_object(scene_object* object, const std::string& name, const transform& transformation);
    void unregister_object(scene_object* object);

    void on_create_render_component(const entt::registry&, const entt::entity entity);
    void on_update_render_component(const entt::registry&, const entt::entity entity);
    void on_destroy_render_component(const entt::registry&, const entt::entity entity);

    void on_create_script_component(const entt::registry&, const entt::entity entity);
    // void on_update_script_component(const entt::registry&, const entt::entity entity);
    void on_destroy_script_component(const entt::registry&, const entt::entity entity);

    void on_create_physics_component(const entt::registry&, const entt::entity entity);
    // void on_update_physics_component(const entt::registry&, const entt::entity entity);
    void on_destroy_physics_component(const entt::registry&, const entt::entity entity);

    void construct_object_from_lua_table(scene_object& scene_obj, sol::table& obj_table);

    bool playing = false;
    scope<scene_storage> storage = nullptr;
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_HPP