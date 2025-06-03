/**
 * @file scene/scene_tree.hpp
 */
#ifndef OTHER_SCENE_SCENE_SCENE_TREE_HPP
#define OTHER_SCENE_SCENE_SCENE_TREE_HPP

#include "core/memory_pool.hpp"
#include "core/ref.hpp"
#include "core/scope.hpp"

#include "object/scene_object.hpp"

namespace other {

  class scene;

  class scene_tree {
   public:
    struct node {
      node* parent = nullptr;
      natural_t id = 0;

      scene_object* object = nullptr;
      std::vector<node*> children = {};
    };

    scene_tree(scene* s);
    ~scene_tree();

    scene_object& create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object = nullptr);
    void destroy_object(natural_t id);

    constexpr static inline size_t kMaxNodes = memory_pool<scene_object>::kMaxObjects;

   private:
    friend class scene;

    scene* scene_ptr = nullptr;

    node* root = nullptr;

    ref<memory_pool<scene_object>> objects = nullptr;
    scope<std::array<node, kMaxNodes>> nodes;

    node* node_at(size_t idx);
    node* node_from_scene_object(const scene_object* object);

    node* create_object(node* parent_node = nullptr);
    void destroy_object(node* n);
  };

}  // namespace other

#endif  // OTHER_SCENE_SCENE_SCENE_TREE_HPP