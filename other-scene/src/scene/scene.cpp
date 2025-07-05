/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include <cstdint>

#include "core/profiler.hpp"
#include "math/morton_codes.hpp"

#include "entt/entity/fwd.hpp"
#include "object/transform.hpp"

namespace other {

  scene::scene() : tree(this) {
    PROFILE_SECTION("scene::scene");
    OTHER_ASSERT(tree.nodes != nullptr, "Scene tree nodes are not initialized.");
    OTHER_ASSERT(tree.objects != nullptr, "Memory pool for scene objects is not initialized.");

    // Create the root object
    scene_object& root = tree.root_object();
    register_object(&root, "Root", glm::vec3(0.0f));
    root.visible = true;
  }

  scene_object& scene::root_object() {
    PROFILE_SECTION("scene::get_root_object");

    scene_tree::node* root_node = tree.node_at(0);
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene tree.");
    OTHER_ASSERT(root_node->object != nullptr, "Root node object is null.");

    return *root_node->object;
  }

  scene_object& scene::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    PROFILE_SECTION("scene::create_object");

    return tree.create_object(name, world_position, parent_object);
  }

  void scene::destroy_object(natural_t id) {
    PROFILE_SECTION("scene::destroy_object");
    tree.destroy_object(id);
  }

  scene_object& scene::get_object(natural_t id) {
    PROFILE_SECTION("scene::get_object");
    scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    return *node->object;
  }

  size_t scene::get_object_count() const {
    PROFILE_SECTION("scene::get_object_count");
    OTHER_ASSERT(tree.nodes != nullptr, "Scene tree nodes are not initialized.");
    return tree.nodes->size();
  }

  transform& scene::get_transform(scene_object* object) {
    PROFILE_SECTION("scene::get_transform");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    transform* t = registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  const transform& scene::get_transform(const scene_object* object) const {
    PROFILE_SECTION("scene::get_transform_const");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    const transform* t = registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  void scene::set_transform(scene_object* object, const transform& t) {
    PROFILE_SECTION("scene::set_transform");
    OTHER_ASSERT(object != nullptr, "Cannot set transform on a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    registry.replace<transform>(entity, t);
  }

  glm::mat4 scene::get_world_transform(scene_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot get world transform from a null scene object.");
    PROFILE_SECTION("scene::get_world_transform");

    return get_world_transform(obj->id);
  }

  glm::mat4 scene::get_world_transform(natural_t id) const {
    PROFILE_SECTION("scene::get_world_transform");

    const scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");

    if (scene_tree::node* parent_node = node->parent; parent_node != nullptr) {
      return get_transform(node->object).world_matrix(get_world_transform(parent_node->id));
    } else {
      return get_transform(node->object).world_matrix();
    }
  }

  transform& scene::get_transform(natural_t id) {
    PROFILE_SECTION("scene::get_transform_by_id");

    scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    return get_transform(node->object);
  }

  const transform& scene::get_transform(natural_t id) const {
    PROFILE_SECTION("scene::get_transform_by_id");

    const scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    return get_transform(node->object);
  }

  void scene::set_transform(natural_t id, const transform& t) {
    PROFILE_SECTION("scene::set_transform_by_id");

    scene_tree::node* node = tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene tree.");
    set_transform(node->object, t);
  }

  std::vector<triangle> scene::get_scene_mesh() const {
    PROFILE_SECTION("scene::get_scene_mesh");

    std::vector<triangle> scene_mesh;
    scene_tree::node* root_node = tree.root;
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene tree.");

    {
      PROFILE_SECTION("scene::get_scene_mesh--collect_mesh_data");
      registry.view<object_handle, transform, render_component>().each([&](const object_handle h, const transform& t, const render_component& render) {
        const std::unordered_map<uint32_t, std::vector<triangle>>& mesh_data = render.model->source->get_triangles();
        for (const auto& [submesh_index, triangles] : mesh_data) {
          scene_mesh.append_range(triangles);
        }
      });
    }

    struct morton_idx {
      uint64_t code;
      size_t index;
    };

    std::vector<triangle> sorted_mesh;
    std::vector<morton_idx> morton_codes(scene_mesh.size());
    {
      PROFILE_SECTION("scene::get_scene_mesh--compute_morton_codes");
      for (uint32_t i = 0; i < (int32_t)scene_mesh.size(); ++i) {
        morton_codes[i].code = morton_encode3d(scene_mesh[i].centroid.x, scene_mesh[i].centroid.y, scene_mesh[i].centroid.z);
        morton_codes[i].index = i;
      }
    }

    std::ranges::sort(morton_codes, [](const morton_idx& a, const morton_idx& b) {
      return a.code < b.code;
    });

    sorted_mesh.resize(scene_mesh.size());
    for (uint32_t i = 0; i < (int32_t)scene_mesh.size(); ++i) {
      sorted_mesh[i] = scene_mesh[morton_codes[i].index];
    }

    return sorted_mesh;
  }

  void scene::render(scope<renderer>& renderer) const {
    PROFILE_SECTION("scene::render");

    OTHER_ASSERT(renderer != nullptr, "Renderer is null, cannot render scene.");
  }

  std::string scene::as_string(const scene& s) {
    PROFILE_SECTION("scene::as_string");

    std::string result = "Scene Object Count: " + std::to_string(s.get_object_count()) + "\n";
    result += "Scene Tree:\n";
    for (size_t i = 0; i < s.tree.nodes->size(); ++i) {
      const scene_tree::node* n = s.tree.node_at(i);
      if (n != nullptr && n->object != nullptr) {
        result += "Node ID: " + std::to_string(n->id) + ", Object Name: " + n->object->name + "\n";
      }
    }
    return result;
  }

  scene::object_handle::operator scene_object*() const {
    OTHER_ASSERT(object != nullptr, "Object handle is null, cannot convert to scene_object*.");
    return object;
  }

  bool scene::object_handle::operator==(const object_handle& other) const {
    return id == other.id && object == other.object;
  }

  void scene::register_object(scene_object* object, const std::string& name, const glm::vec3& world_position) {
    PROFILE_SECTION("scene::register_object");

    OTHER_ASSERT(object != nullptr, "Cannot register a null scene object.");

    entt::entity entity = registry.create();
    registry.emplace<object_handle>(entity, object_handle{ .id = (natural_t)entity, .object = object });
    transform& t = registry.emplace<transform>(entity);
    t = {
      .local_basis = orthonormal_basis(glm::vec3(0, 1, 0)),
      .local_position = world_position,
      .local_scale = glm::vec3(1, 1, 1),
      .local_rotation_quat = glm::quat(1, 0, 0, 0),
    };

    object->name = name;
    object->registry_id = (uint32_t)entity;
  }

  void scene::unregister_object(scene_object* object) {
    PROFILE_SECTION("scene::unregister_object");

    OTHER_ASSERT(object != nullptr, "Cannot unregister a null scene object.");
    if (object->registry_id != 0) {
      registry.destroy(entt::entity(object->registry_id));
    }
  }

  void scene::on_create_render_component(render_component& render, const entt::registry&, const entt::entity entity) {
  }

  void scene::on_update_render_component(render_component& render, const entt::registry&, const entt::entity entity) {
  }

  void scene::on_destroy_render_component(render_component& render, const entt::registry&, const entt::entity entity) {
  }

}  // namespace other