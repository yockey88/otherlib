/**
 * \file scene/scene_tree.cpp
 **/
#include "scene/scene_tree.hpp"

#include "model/vertex.hpp"
#include "object/scene_object.hpp"

#include "scene/scene.hpp"

namespace other {

  scene_tree::scene_tree()
      : scene_ptr(nullptr), objects(make_ref<memory_pool<scene_object>>()), nodes(make_scope<std::array<node, kMaxNodes>>()) {
    OTHER_ASSERT(objects != nullptr, "Failed to allocate memory pool for scene objects.");

    root = create_object();
    OTHER_ASSERT(root != nullptr, "Failed to create root node in scene tree.");
  }

  scene_tree::scene_tree(scene* s)
      : scene_ptr(s), objects(make_ref<memory_pool<scene_object>>()), nodes(make_scope<std::array<node, kMaxNodes>>()) {
    OTHER_ASSERT(objects != nullptr, "Failed to allocate memory pool for scene objects.");

    root = create_object();
    OTHER_ASSERT(root != nullptr, "Failed to create root node in scene tree.");
  }

  scene_tree::scene_tree(scene_tree&& other) {
    *this = std::move(other);
  }

  scene_tree& scene_tree::operator=(scene_tree&& other) {
    this->scene_ptr = other.scene_ptr;
    other.scene_ptr = nullptr;

    this->objects = other.objects;
    other.objects = nullptr;

    this->nodes = std::move(other.nodes);
    other.nodes = nullptr;

    this->root = other.root;
    other.root = nullptr;

    this->num_objects = other.num_objects;
    other.num_objects = 0;

    return *this;
  }

  scene_tree::~scene_tree() {
    if (objects != nullptr) {
      objects->clear();
      objects = nullptr;
    }

    if (nodes != nullptr) {
      // Clear nodes
      for (auto& node : *nodes) {
        node.parent = nullptr;
        node.id = 0;
        node.object = nullptr;
        node.children.clear();
      }
      nodes = nullptr;
    }

    root = nullptr;  // Clear root pointer
  }

  scene_object& scene_tree::root_object() {
    OTHER_ASSERT(root != nullptr, "Root node is null, cannot access root object.");
    OTHER_ASSERT(root->object != nullptr, "Root node object is null, cannot access root object.");
    return *root->object;
  }

  scene_object& scene_tree::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    PROFILE_SECTION("scene_tree::create_object");

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
    ++num_objects;

    CORE_LOG_DEBUG("Created scene object : \n{}", type_data_handler<scene_object>::as_string("object", *new_node->object));
    return *new_node->object;
  }

  scene_object& scene_tree::add_object(scene_object* object, const transform& transformation, scene_object* parent_object) {
    PROFILE_SECTION("scene_tree::add_object");

    OTHER_ASSERT(object != nullptr, "Cannot add a null scene object.");
    OTHER_ASSERT(scene_ptr != nullptr, "Scene pointer is null, cannot add object.");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    node* parent_node = nullptr;
    if (parent_object != nullptr) {
      parent_node = node_from_scene_object(parent_object);
      OTHER_ASSERT(parent_node != nullptr, "Parent object not found in scene tree.");
    }

    node* new_node = create_object(parent_node);
    OTHER_ASSERT(new_node != nullptr, "Failed to create new node in scene tree.");

    *new_node->object = *object;
    scene_ptr->register_object(new_node->object, object->name, transformation);
    ++num_objects;

    CORE_LOG_DEBUG("Added scene object : \n{}", type_data_handler<scene_object>::as_string("object", *new_node->object));
    return *new_node->object;
  }

  scene_object* scene_tree::get_parent(natural_t id) {
    PROFILE_SECTION("scene_tree::get_parent");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");
    OTHER_ASSERT(id < kMaxNodes, "ID out of bounds for scene tree nodes.");

    node* target_node = node_at(id);
    OTHER_ASSERT(target_node != nullptr, "Node with ID {} not found in scene tree.", id);

    if (target_node->parent != nullptr && target_node->parent->object != nullptr) {
      return target_node->parent->object;
    }
    return nullptr;
  }

  const scene_object* scene_tree::get_parent(natural_t id) const {
    return const_cast<scene_tree*>(this)->get_parent(id);
  }

  scene_object* scene_tree::get_parent(scene_object* object) {
    PROFILE_SECTION("scene_tree::get_parent");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  const scene_object* scene_tree::get_parent(const scene_object* object) const {
    PROFILE_SECTION("scene_tree::get_parent_const");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  void scene_tree::destroy_object(natural_t id) {
    PROFILE_SECTION("scene_tree::destroy_object");

    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");
    OTHER_ASSERT(id < kMaxNodes, "ID out of bounds for scene tree nodes.");

    node* target_node = node_at(id);
    OTHER_ASSERT(target_node != nullptr, "Node with ID {} not found in scene tree.", id);

    destroy_object(target_node);
  }

  std::vector<uint64_t> scene_tree::get_all_object_ids() const {
    PROFILE_SECTION("scene_tree::get_all_object_ids");

    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");
    std::vector<uint64_t> ids;
    for (const auto& node : *nodes) {
      if (node.object != nullptr && node.object->id != 0) {
        ids.push_back(node.object->id);
      }
    }
    return ids;
  }

  size_t scene_tree::get_object_count() const {
    PROFILE_SECTION("scene_tree::get_object_count");

    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");
    return num_objects;
  }

  scene_tree::node* scene_tree::node_at(size_t idx) {
    PROFILE_SECTION("scene_tree::node_at");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    OTHER_ASSERT(idx < kMaxNodes, "Index out of bounds for scene tree nodes.");
    return &nodes->at(idx);
  }

  const scene_tree::node* scene_tree::node_at(size_t idx) const {
    return const_cast<scene_tree*>(this)->node_at(idx);
  }

  bool scene_tree::node_has_tag(size_t idx, const std::string_view tag) const {
    PROFILE_SECTION("scene_tree::node_has_tag");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    const node* n = node_at(idx);
    OTHER_ASSERT(n != nullptr, "Node with ID {} not found in scene tree.", idx);
    return std::any_of(n->tags.begin(), n->tags.end(), [&](const object_tag& t) { return t.name == tag; });
  }

  scene_tree::node* scene_tree::node_from_scene_object(const scene_object* object) {
    PROFILE_SECTION("scene_tree::node_from_scene_object");

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

  const scene_tree::node* scene_tree::node_from_scene_object(const scene_object* object) const {
    return const_cast<scene_tree*>(this)->node_from_scene_object(object);
  }

  scene_tree::node* scene_tree::create_object(node* parent_node) {
    PROFILE_SECTION("scene_tree::create_object");

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
    PROFILE_SECTION("scene_tree::destroy_object");

    OTHER_ASSERT(n != nullptr, "Node pointer is null, cannot destroy object.");
    OTHER_ASSERT(objects != nullptr, "Memory pool for scene objects is not initialized.");
    OTHER_ASSERT(nodes != nullptr, "Node array is not initialized.");

    if (n->parent != nullptr) {
      auto& siblings = n->parent->children;
      std::erase_if(siblings, [n](node* child) { return child == n; });
    }

    if (n->object != nullptr) {
      scene_ptr->unregister_object(n->object);
      objects->free(n->object->id);
      *n = node{};  // Reset the node
      --num_objects;
    }
  }

}  // namespace other