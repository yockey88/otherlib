/**
 * @file scene/scene_tree.hpp
 */
#ifndef OTHER_SCENE_SCENE_SCENE_TREE_HPP
#define OTHER_SCENE_SCENE_SCENE_TREE_HPP

#include "core/memory_pool.hpp"
#include "core/ref.hpp"
#include "core/scope.hpp"
#include "math/bounding_box.hpp"

#include "model/vertex.hpp"

#include "object/scene_object.hpp"
#include "object/transform.hpp"

#include "glm/fwd.hpp"

namespace other {

  class scene;

  struct object_tag {
    // std::string type;
    std::string name;
  };

  namespace builtin_tags {
    namespace types {

    }  // namespace types

  }  // namespace builtin_tags

  class scene_tree {
   public:
    struct node {
      node* parent = nullptr;
      natural_t id = 0;
      bool is_leaf = true;

      // contains object and all children nodes
      bounding_box bbox = {};
      scene_object* object = nullptr;
      std::vector<node*> children = {};

      std::vector<object_tag> tags = {};
    };

    scene_tree();
    scene_tree(scene* s);

    scene_tree(scene_tree&&);
    scene_tree& operator=(scene_tree&&);

    scene_tree(const scene_tree&) = delete;
    scene_tree& operator=(const scene_tree&) = delete;

    ~scene_tree();

    void destroy_all_objects();

    scene_object& root_object();
    scene_object& create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object = nullptr);

    scene_object& add_object(scene_object* object, const transform& transformation, scene_object* parent_object = nullptr);

    scene_object* find_object_by_id(natural_t id) const;
    scene_object* find_object_by_name(const std::string_view name) const;

    scene_object* get_parent(natural_t id);
    const scene_object* get_parent(natural_t id) const;
    scene_object* get_parent(scene_object* object);
    const scene_object* get_parent(const scene_object* object) const;

    void destroy_object(natural_t id);

    std::vector<uint64_t> get_all_object_ids() const;

    size_t get_object_count() const;

    constexpr static inline size_t kMaxNodes = memory_pool<scene_object>::kMaxObjects;

   private:
    friend class scene;

    scene* scene_ptr = nullptr;

    node* root = nullptr;

    size_t num_objects = 0;
    scope<memory_pool<scene_object, kMaxNodes>> objects = nullptr;

    std::array<uint32_t, kMaxNodes> generation_counters;
    scope<std::array<node, kMaxNodes>> nodes = nullptr;

    node* node_at(size_t idx);
    const node* node_at(size_t idx) const;
    bool node_has_tag(size_t idx, const std::string_view tag) const;

    node* node_from_scene_object(const scene_object* object);
    const node* node_from_scene_object(const scene_object* object) const;

    node* create_object(node* parent_node = nullptr);
    void destroy_object(node* n);
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_TREE_HPP