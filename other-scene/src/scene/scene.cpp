/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include <cstdint>

#include "core/profiler.hpp"

#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "renderer/camera.hpp"
#include "script/scripting_environment.hpp"

#include "entt/entity/fwd.hpp"

namespace other {

  scene::scene()
      : tree(this) {
    PROFILE_SECTION("scene::scene");
    OTHER_ASSERT(tree.nodes != nullptr, "Scene tree nodes are not initialized.");
    OTHER_ASSERT(tree.objects != nullptr, "Memory pool for scene objects is not initialized.");

    // Create the root object
    scene_object& root = tree.root_object();
    register_object(&root, "Root", glm::vec3(0.0f));
    root.visible = true;

    registry.on_construct<script_component>().connect<&scene::on_create_script_component>(this);
    // registry.on_update<script_component>().connect<&scene::on_update_script_component>(this);
    registry.on_destroy<script_component>().connect<&scene::on_destroy_script_component>(this);
  }

  scene::~scene() {
    PROFILE_SECTION("scene::~scene");

    registry.on_construct<script_component>().disconnect<&scene::on_create_script_component>(this);
    // registry.on_update<script_component>().disconnect<&scene::on_update_script_component>(this);
    registry.on_destroy<script_component>().disconnect<&scene::on_destroy_script_component>(this);
    registry.clear();
  }

  scene_object& scene::root_object() {
    PROFILE_SECTION("scene::get_root_object");

    scene_tree::node* root_node = tree.node_at(0);
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene tree.");
    OTHER_ASSERT(root_node->object != nullptr, "Root node object is null.");

    return *root_node->object;
  }

  scene_object& scene::create_object(const std::string& name, scene_object* parent_object) {
    return create_object(name, glm::vec3(0.f), parent_object);
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
    return tree.get_object_count();
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

    {
      PROFILE_SECTION("scene::get_world_transform--recursive-compute");
      if (scene_tree::node* parent_node = node->parent; parent_node != nullptr) {
        return get_transform(node->object).world_matrix(get_world_transform(parent_node->id));
      } else {
        return get_transform(node->object).world_matrix();
      }
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

  render_data scene::prepare_render_data() const {
    PROFILE_SECTION("scene::prepare_render_data");

    render_data data;
    const camera* primary_camera = nullptr;
    registry.view<object_handle, camera>().each([&](const object_handle& handle, const camera& cam) {
      if (object_has_tag(handle.id, "main-camera")) {
        primary_camera = &cam;
      }
    });
    if (primary_camera != nullptr) {
      /// \todo fix this const cast
      data.primary_camera = (camera*)primary_camera;
    }

    /// collect lights
    /// \todo: should we collect these into an owning group?
    //        pros: faster, faster, faster, and then also a little bit faster
    //        cons: have to remember to create the groups and two entities can not have one of each light
    registry.view<gpu::point_light>().each([&](const gpu::point_light& light) { data.point_lights.push_back(light); });
    registry.view<object_handle, gpu::directional_light>().each([&](const object_handle& handle, const gpu::directional_light& light) {
      if (object_has_tag(handle.id, "scene-ambient-light")) {
        data.scene_ambient_light = &light;
      }
    });
    registry.view<object_handle, render_component>().each([&](const object_handle& handle, const render_component& render) {
      if (!render.visible) {
        return;
      }

      model* draw_model = render.model;
      OTHER_ASSERT(draw_model != nullptr, "Draw command model is null");
      OTHER_ASSERT(draw_model->source != nullptr, "Draw command model source is null");
      PROFILE_SECTION("scene::prepare_render_data--submit_model");

      model_source* source = draw_model->source;
      OTHER_ASSERT(source != nullptr, "Model source is null");

      const std::vector<submesh>& submeshes = source->get_submeshes();
      OTHER_ASSERT(!submeshes.empty(), "Model source has no submeshes");

      const std::vector<uint32_t>& sm_idxs = draw_model->submesh_indices;
      OTHER_ASSERT(!sm_idxs.empty(), "Model has no submeshes");
      for (const auto& sm_idx : sm_idxs) {
        OTHER_ASSERT(sm_idx < submeshes.size(), "Submesh index out of bounds");
        draw_command cmd = {
          .draw_model = draw_model,
          .transform = get_world_transform(handle.id) * submeshes[sm_idx].local_transform,
          .material = render.material,
          .submesh_index = sm_idx,
          .render_state = render_polygon_mode::POLYGON_MODE_FILL,
          .draw_mode = mesh::primitive_type::TRIANGLES,
          .line_thickness = 1.f,
        };

        mesh_key key = cmd;

        auto it = data.mesh_indices.find(key);
        if (it == data.mesh_indices.end()) {
          auto [itr, inserted] = data.mesh_indices.insert({ key, data.num_draw_calls++ });
          OTHER_ASSERT(inserted, "Failed to insert mesh key into map");

          data.mesh_keys.emplace_back() = key;
          data.draw_calls.emplace_back() = draw_call{};
          data.material_buffers.emplace_back() = gpu::graphics_material_buffer{};
          data.model_buffers.emplace_back() = gpu::model_matrix_buffer{};
          it = itr;
        }
        size_t mesh_index = it->second;

        draw_call& call = data.draw_calls[mesh_index];
        const submesh& sm = cmd.draw_model->source->get_submeshes()[cmd.submesh_index];
        if (call.instance_count == 0) {
          call.instance_count = 0;
          call.submesh_index = cmd.submesh_index;

          call.mesh_handle = cmd.draw_model->source->get_mesh_handle();
          call.submesh_index = cmd.submesh_index;

          call.vertex_offset = sm.base_vertex;
          call.vertex_count = sm.vert_cnt;
          call.index_offset = sm.base_idx;
          call.index_count = sm.idx_cnt;

          call.line_thickness = cmd.line_thickness;
        }

        size_t index = data.draw_calls[mesh_index].instance_count++;
        data.material_buffers[mesh_index].materials[index] = cmd.material;
        data.model_buffers[mesh_index].model_matrices[index] = cmd.transform;
      }
    });
    return data;
  }

  bool scene::object_has_tag(natural_t id, const std::string_view tag) const {
    PROFILE_SECTION("scene::object_has_tag");
    return tree.node_has_tag(id, tag);
  }

  void scene::add_object_tag(natural_t id, const std::string_view tag) {
    PROFILE_SECTION("scene::add_object_tag");
    scene_tree::node* n = tree.node_at(id);
    OTHER_ASSERT(n != nullptr, "Node with the given ID does not exist in the scene tree.");
    n->tags.push_back(object_tag{ std::string{ tag } });
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
    object->name = name;
    object->registry_id = (uint32_t)entity;

    registry.emplace<object_handle>(entity, object_handle{ .id = (natural_t)entity, .object = object });
    registry.emplace<script_component>(entity, script_component{ .object = object });
    registry.emplace<transform>(entity, transform{
                                          .local_basis = orthonormal_basis(glm::vec3(0, 1, 0)),
                                          .local_position = world_position,
                                          .local_scale = glm::vec3(1, 1, 1),
                                          .local_rotation_quat = glm::quat(1, 0, 0, 0),
                                        });
  }

  void scene::unregister_object(scene_object* object) {
    PROFILE_SECTION("scene::unregister_object");

    OTHER_ASSERT(object != nullptr, "Cannot unregister a null scene object.");
    if (object->registry_id != 0) {
      registry.destroy(entt::entity(object->registry_id));
    }
  }

  void scene::on_create_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_update_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_destroy_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_create_script_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_create_script_component");

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script_component& script = registry.get<script_component>(entity);
    std::string script_name = script.object->name;
    script.script_object_id = script_env->create_object(script_name);
  }

  // void scene::on_update_script_component(const entt::registry&, const entt::entity entity) {
  // }

  void scene::on_destroy_script_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_destroy_script_component");

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script_component& script = registry.get<script_component>(entity);
    // script_env->destroy_object(script.script_object_id);
  }

}  // namespace other