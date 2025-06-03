/**
 * \file scene/scene_tree.cpp
 **/
#include "scene/scene_tree.hpp"

#include "scene/scene.hpp"

#include "object/scene_object.hpp"

namespace other {

  scene_tree::scene_tree(scene* s)
      : scene_ptr(s), objects(make_ref<memory_pool<scene_object>>()), nodes(make_scope<std::array<node, kMaxNodes>>()) {
    OTHER_ASSERT(objects != nullptr, "Failed to allocate memory pool for scene objects.");

    root = create_object();
    OTHER_ASSERT(root != nullptr, "Failed to create root node in scene tree.");
  }

  scene_tree::~scene_tree() {
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    if (objects != nullptr) {
      objects->clear();
      objects = nullptr;
    }

    // Clear nodes
    for (auto& node : *nodes) {
      node.parent = nullptr;
      node.id = 0;
      node.object = nullptr;
      node.children.clear();
    }

    root = nullptr;  // Clear root pointer
  }

  scene_object& scene_tree::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null, cannot create object.");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    node* parent_node = nullptr;
    if (parent_object != nullptr) {
      parent_node = node_from_scene_object(parent_object);
      OTHER_ASSERT(parent_node != nullptr, "Parent object not found in scene tree.");
    }

    node* new_node = create_object(parent_node);
    OTHER_ASSERT(new_node != nullptr, "Failed to create new node in scene tree.");

    scene_ptr->register_object(new_node->object, name, world_position);
    return *new_node->object;
  }

  void scene_tree::destroy_object(natural_t id) {
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");
    OTHER_ASSERT(id < kMaxNodes, "ID out of bounds for scene tree nodes.");

    node* target_node = node_at(id);
    OTHER_ASSERT(target_node != nullptr, "Node with ID {} not found in scene tree.", id);

    scene_ptr->unregister_object(target_node->object);
    destroy_object(target_node);
    OTHER_ASSERT(target_node->object == nullptr, "Node object was not cleared after destruction.");
    OTHER_ASSERT(target_node->id == 0, "Node ID was not reset after destruction.");
    OTHER_ASSERT(target_node->parent == nullptr, "Node parent was not cleared after destruction.");
    OTHER_ASSERT(target_node->children.empty(), "Node children were not cleared after destruction.");
  }

  scene_tree::node* scene_tree::node_at(size_t idx) {
    OTHER_ASSERT(idx < kMaxNodes, "Index out of bounds for scene tree nodes.");
    return &nodes->at(idx);
  }

  scene_tree::node* scene_tree::node_from_scene_object(const scene_object* object) {
    OTHER_ASSERT(object != nullptr, "Scene object pointer is null, cannot find node.");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    for (auto& node : *nodes) {
      if (node.object == object) {
        return &node;
      }
    }
    return nullptr;
  }

  scene_tree::node* scene_tree::create_object(node* parent_node) {
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    auto [obj, idx] = objects->emplace();
    OTHER_ASSERT(idx < kMaxNodes, "Exceeded maximum number of nodes in scene tree.");
    OTHER_ASSERT(idx < nodes->size(), "Exceeded maximum number of nodes in scene tree.");

    node* node_ptr = node_at(idx);
    node_ptr->parent = parent_node;
    node_ptr->id = idx;
    node_ptr->object = &obj;

    /// these have to stay in sync with each other because they reference the index in the memory pool
    node_ptr->object->id = node_ptr->id;

    if (parent_node != nullptr) {
      parent_node->children.push_back(node_at(idx));
    }
    return node_ptr;
  }

  void scene_tree::destroy_object(node* n) {
    OTHER_ASSERT(n != nullptr, "Node pointer is null, cannot destroy object.");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    if (n->parent != nullptr) {
      auto& siblings = n->parent->children;
      std::ranges::remove_if(siblings, [n](node* child) { return child == n; });
    }

    if (n->object != nullptr) {
      scene_ptr->unregister_object(n->object);
      objects->free(n->object->id);
      *n = node{};  // Reset the node
    }
  }

}  // namespace other