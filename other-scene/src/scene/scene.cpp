/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include <cstdint>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "model/model.hpp"
#include "model/skeleton.hpp"
#include "renderer/camera.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_controller.hpp"
#include "object/object_serialization_data.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"

#include "entt/entity/fwd.hpp"
#include "scene_storage.hpp"

namespace other {

  void scene::scene_first_construction_initialization() {
    storage = make_scene_storage(this);

    // Create the root object
    scene_object& root = storage->tree.root_object();
    register_object(&root, "Root", glm::vec3(0.0f));
    root.visible = true;
  }

  void scene::do_final_scene_destruction_cleanup() {
    storage->tree.destroy_all_objects();
    storage = nullptr;
  }

  void scene::do_scene_binding() {
    storage->registry.on_construct<script_component>().connect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().connect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().connect<&scene::on_destroy_script_component>(this);
  }

  void scene::do_scene_unbinding() {
    OTHER_ASSERT(storage != nullptr, "Scene storage is not initialized.");
    storage->registry.on_construct<script_component>().disconnect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().disconnect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().disconnect<&scene::on_destroy_script_component>(this);
  }

  scene::scene() {
    scene_first_construction_initialization();
    do_scene_binding();
  }

  scene::scene(const std::string_view name) {
    this->name = name;
    this->id = FNV(name);
    scene_first_construction_initialization();
    do_scene_binding();
  }

  scene::scene(scene&& other) {
    *this = std::move(other);
  }

  scene& scene::operator=(scene&& other) {
    if (this == &other) {
      return *this;
    }

    this->name = std::move(other.name);
    this->id = other.id;
    other.id = 0;
    other.name = "Untitled Scene";

    /// unbind them from the other scene first
    other.do_scene_unbinding();
    this->storage = std::move(other.storage);
    other.storage = nullptr;
    /// then bind to this scene
    do_scene_binding();

    /// transfer tree pointer (this probably needs to be done better)
    this->storage->tree.scene_ptr = this;
    return *this;
  }

  scene::~scene() {
    if (storage != nullptr) {
      do_scene_unbinding();
      do_final_scene_destruction_cleanup();
    }
  }

  void scene::reset() {
  }

  scene scene::create_scene(const std::string& name) {
    return scene(name);
  }

  void scene::fixed_update(double delta_time) {
    PROFILE_SECTION("scene::fixed_update");

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      // comp.fixed_update(delta_time);
    });
  }

  void scene::update(double delta_time) {
    PROFILE_SECTION("scene::update");

    storage->registry.view<render_component>().each([delta_time](entt::entity entity, render_component& render_comp) {
      // if (!render_comp.animated) {
      //   auto* model_ptr = render_comp.model;
      //   model_ptr->bone_matrices = model_ptr->skel->calculate_bone_matrices(glm::mat4(1.0f));
      // }
    });
    storage->registry.view<object_handle, animation_controller>().each([this, delta_time](entt::entity entity, object_handle& obj_handle, animation_controller& anim_ctrl) {
      anim_ctrl.root_transform = get_world_transform(obj_handle.id);
      anim_ctrl.update(delta_time);
    });

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      // comp.update(delta_time);
    });
  }

  void scene::late_update(double delta_time) {
    PROFILE_SECTION("scene::late_update");

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      // comp.late_update(delta_time);
    });
  }

  scene_object& scene::root_object() {
    PROFILE_SECTION("scene::get_root_object");

    scene_tree::node* root_node = storage->tree.node_at(0);
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene storage->tree.");
    OTHER_ASSERT(root_node->object != nullptr, "Root node object is null.");

    return *root_node->object;
  }

  scene_object& scene::create_object(scene_object* object) {
    PROFILE_SECTION("scene::create_object_from_existing");
    OTHER_ASSERT(object != nullptr, "Cannot create a scene object from a null pointer.");
    return storage->tree.create_object(object->name, glm::vec3(0.f), nullptr);
  }

  scene_object& scene::create_object(const std::string& name, scene_object* parent_object) {
    PROFILE_SECTION("scene::create_object_with_name");
    return create_object(name, glm::vec3(0.f), parent_object);
  }

  scene_object& scene::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    PROFILE_SECTION("scene::create_object");
    return storage->tree.create_object(name, world_position, parent_object);
  }

  scene_object& scene::add_object(scene_object* object, const transform& transformation, scene_object* parent_object) {
    PROFILE_SECTION("scene::add_object");
    return storage->tree.add_object(object, transformation, parent_object);
  }

  void scene::add_objects(const std::span<serialization::parsed_scene_object> objects) {
    PROFILE_SECTION("scene::add_objects");
    for (serialization::parsed_scene_object& obj : objects) {
      add_object(&obj.object, obj.obj_transform, &get_object(obj.parent_id));

      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Failed to retrieve scripting environment");

      script_component* comp = get_component<script_component>(&obj.object);
      OTHER_ASSERT(comp != nullptr, "Failed to retrieve script component for object w/ id {}", obj.object.id);

      if (obj.dotnet_obj.name != "" && obj.dotnet_obj.dotnet_blob.size() > 0) {
        CORE_LOG_DEBUG(" - attaching serialized .NET object '{}' to script object ID {}", obj.dotnet_obj.name, comp->script_object_id);
        env->attach_serialized_dotnet_object(comp->script_object_id, obj.dotnet_obj.name, obj.dotnet_obj.dotnet_blob);
      }
    }
  }

  scene_object* scene::get_parent(natural_t id) {
    PROFILE_SECTION("scene::get_parent");
    return storage->tree.get_parent(id);
  }

  const scene_object* scene::get_parent(natural_t id) const {
    PROFILE_SECTION("scene::get_parent_const");
    return const_cast<scene*>(this)->get_parent(id);
  }

  scene_object* scene::get_parent(scene_object* object) {
    PROFILE_SECTION("scene::get_parent");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  const scene_object* scene::get_parent(const scene_object* object) const {
    PROFILE_SECTION("scene::get_parent_const");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  std::vector<uint64_t> scene::get_children_ids(natural_t id) const {
    PROFILE_SECTION("scene::get_children_ids");
    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    std::vector<uint64_t> children_ids;
    for (const scene_tree::node* child : node->children) {
      if (child != nullptr && child->object != nullptr) {
        children_ids.push_back(child->object->id);
      }
    }
    return children_ids;
  }

  std::vector<uint64_t> scene::get_children_ids(const scene_object* object) const {
    PROFILE_SECTION("scene::get_children_ids");
    if (object == nullptr) {
      return {};
    }
    return get_children_ids(object->id);
  }

  std::vector<uint64_t> scene::get_all_object_ids() const {
    PROFILE_SECTION("scene::get_all_object_ids");
    return storage->tree.get_all_object_ids();
  }

  void scene::destroy_object(natural_t id) {
    PROFILE_SECTION("scene::destroy_object");
    storage->tree.destroy_object(id);
  }

  scene_object& scene::get_object(natural_t id) {
    PROFILE_SECTION("scene::get_object");
    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return *node->object;
  }

  const scene_object& scene::get_object(natural_t id) const {
    PROFILE_SECTION("scene::get_object_const");
    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return *node->object;
  }

  size_t scene::get_object_count() const {
    PROFILE_SECTION("scene::get_object_count");
    OTHER_ASSERT(storage->tree.nodes != nullptr, "Scene tree nodes are not initialized.");
    return storage->tree.get_object_count();
  }

  transform& scene::get_transform(scene_object* object) {
    PROFILE_SECTION("scene::get_transform");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    transform* t = storage->registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  const transform& scene::get_transform(const scene_object* object) const {
    PROFILE_SECTION("scene::get_transform_const");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    const transform* t = storage->registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  void scene::set_transform(scene_object* object, const transform& t) {
    PROFILE_SECTION("scene::set_transform");
    OTHER_ASSERT(object != nullptr, "Cannot set transform on a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    storage->registry.replace<transform>(entity, t);
  }

  glm::mat4 scene::get_world_transform(scene_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot get world transform from a null scene object.");
    PROFILE_SECTION("scene::get_world_transform");

    return get_world_transform(obj->id);
  }

  glm::mat4 scene::get_world_transform(natural_t id) const {
    PROFILE_SECTION("scene::get_world_transform");

    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");

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

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return get_transform(node->object);
  }

  const transform& scene::get_transform(natural_t id) const {
    PROFILE_SECTION("scene::get_transform_by_id");

    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return get_transform(node->object);
  }

  void scene::set_transform(natural_t id, const transform& t) {
    PROFILE_SECTION("scene::set_transform_by_id");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    set_transform(node->object, t);
  }

  render_data scene::prepare_render_data() const {
    PROFILE_SECTION("scene::prepare_render_data");

    render_data data;
    const camera* primary_camera = nullptr;
    storage->registry.view<object_handle, camera>().each([&](const object_handle& handle, const camera& cam) {
      if (object_has_tag(handle.id, "main-camera")) {
        primary_camera = &cam;
      }
    });

    if (primary_camera != nullptr) {
      /// \todo fix this const cast
      data.primary_camera = (camera*)primary_camera;
    }

    storage->registry.view<gpu::point_light>().each([&](const gpu::point_light& light) { data.point_lights.push_back(light); });
    storage->registry.view<object_handle, gpu::directional_light>().each([&](const object_handle& handle, const gpu::directional_light& light) {
      if (object_has_tag(handle.id, "scene-ambient-light")) {
        data.scene_ambient_light = &light;
      }
    });

    storage->registry.view<object_handle, render_component>().each([&](const object_handle& handle, const render_component& render) {
      if (!render.visible) {
        return;
      }

      model* draw_model = render.model;
      OTHER_ASSERT(draw_model != nullptr, "Draw command model is null");
      OTHER_ASSERT(draw_model->source != nullptr, "Draw command model source is null");
      PROFILE_SECTION("scene::prepare_render_data--submit_model");

      const animation_controller* anim_ctrl = nullptr;
      if (has_component<animation_controller>(handle.id)) {
        anim_ctrl = get_component<animation_controller>(handle.id);
      }

      const std::vector<submesh>& submeshes = draw_model->source->get_submeshes();
      OTHER_ASSERT(!submeshes.empty(), "Model source has no submeshes");

      const std::vector<uint32_t>& sm_idxs = draw_model->submesh_indices;
      OTHER_ASSERT(!sm_idxs.empty(), "Model has no submeshes");
      for (const auto& sm_idx : sm_idxs) {
        OTHER_ASSERT(sm_idx < submeshes.size(), "Submesh index out of bounds");

        auto transform_it = draw_model->local_submesh_transforms.find(sm_idx);
        OTHER_ASSERT(transform_it != draw_model->local_submesh_transforms.end(), "Local submesh transform not found for submesh index {}", sm_idx);

        mesh_key key = {
          .model_source_handle = draw_model->source->get_mesh_handle(),
          .render_state = render_polygon_mode::POLYGON_MODE_FILL,
          .draw_mode = mesh::primitive_type::TRIANGLES,
          .submesh_index = sm_idx,
        };

        auto it = data.mesh_indices.find(key);
        if (it == data.mesh_indices.end()) {
          auto [itr, inserted] = data.mesh_indices.insert({ key, data.num_draw_calls++ });
          OTHER_ASSERT(inserted, "Failed to insert mesh key into map");

          data.mesh_keys.emplace_back() = key;
          data.draw_calls.emplace_back() = draw_call{};
          data.material_buffers.emplace_back() = gpu::graphics_material_buffer{};
          data.model_buffers.emplace_back() = gpu::model_matrix_buffer{};
          data.bone_buffers.emplace_back() = gpu::bone_matrix_buffer{};

          it = itr;
        }
        OTHER_ASSERT(it != data.mesh_indices.end(), "Mesh key not found in map after insertion");

        size_t mesh_index = it->second;

        draw_call& call = data.draw_calls[mesh_index];
        const submesh& sm = draw_model->source->get_submeshes()[sm_idx];
        if (call.instance_count == 0) {
          call.instance_count = 0;
          call.submesh_index = sm_idx;

          call.mesh_handle = draw_model->source->get_mesh_handle();
          call.submesh_index = sm_idx;

          call.vertex_offset = sm.base_vertex;
          call.vertex_count = sm.vert_cnt;
          call.index_offset = sm.base_idx;
          call.index_count = sm.idx_cnt;

          call.line_thickness = 1.f;
        }

        glm::mat4 world_transform = get_world_transform(handle.id);  // * transform_it->second;

        size_t index = data.draw_calls[mesh_index].instance_count++;
        data.material_buffers[mesh_index].materials[index] = render.material;
        data.model_buffers[mesh_index].model_matrices[index] = world_transform;
      }

      for (auto& bone_buff : data.bone_buffers) {
        for (size_t i = 0; i < gpu::kMaxMaterials; ++i) {
          bone_buff.bone_matrices[i] = glm::mat4(1.0f);
        }
        if (!draw_model->skel || draw_model->bone_matrices.size() == 0) {
          std::ranges::fill(std::span(bone_buff.bone_matrices, gpu::kMaxMaterials), glm::mat4(1.0f));
        } else {
          size_t bone_count = std::min(draw_model->bone_matrices.size(), static_cast<size_t>(100));
          for (size_t b = 0; b < bone_count; ++b) {
            bone_buff.bone_matrices[b] = draw_model->bone_matrices[b];
          }
          draw_model->bone_matrices.clear();
        }
      }
    });

    return data;
  }

  bool scene::object_has_tag(natural_t id, const std::string_view tag) const {
    PROFILE_SECTION("scene::object_has_tag");
    return storage->tree.node_has_tag(id, tag);
  }

  void scene::add_object_tag(natural_t id, const std::string_view tag) {
    PROFILE_SECTION("scene::add_object_tag");
    scene_tree::node* n = storage->tree.node_at(id);
    OTHER_ASSERT(n != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    n->tags.push_back(object_tag{ std::string{ tag } });
  }

  std::string scene::as_string(const scene& s) {
    PROFILE_SECTION("scene::as_string");

    std::string result = "Scene Object Count: " + std::to_string(s.get_object_count()) + "\n";
    result += "Scene Tree:\n";
    for (size_t i = 0; i < s.storage->tree.nodes->size(); ++i) {
      const scene_tree::node* n = s.storage->tree.node_at(i);
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

    entt::entity entity = storage->registry.create();
    object->name = name;
    object->registry_id = (uint32_t)entity;

    storage->registry.emplace<object_handle>(entity, object_handle{ .id = (natural_t)entity, .object = object });
    storage->registry.emplace<transform>(entity, transform{
                                                   .local_basis = orthonormal_basis(glm::vec3(0, 1, 0)),
                                                   .local_position = world_position,
                                                   .local_scale = glm::vec3(1, 1, 1),
                                                   .local_rotation_quat = glm::quat(1, 0, 0, 0),
                                                 });
    storage->registry.emplace<script_component>(entity, script_component{ .object = object });
  }

  void scene::register_object(scene_object* object, const std::string& name, const transform& transformation) {
    register_object(object, name, transformation.local_position);
    set_transform(object, transformation);
  }

  void scene::unregister_object(scene_object* object) {
    PROFILE_SECTION("scene::unregister_object");

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script_component& script = storage->registry.get<script_component>(entt::entity(object->registry_id));
    script_env->destroy_object(script.script_object_id);
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

    script_component* script = storage->registry.try_get<script_component>(entity);
    OTHER_ASSERT(script != nullptr, "Script component is null for entity {}", (natural_t)entity);

    std::string script_name = script->object->name;
    script->script_object_id = script_env->create_object(script_name);

    // script_env->attach_dotnet_object(script.script_object_id, "Other.SceneObject");
  }

  // void scene::on_update_script_component(const entt::registry&, const entt::entity entity) {
  // }

  void scene::on_destroy_script_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_destroy_script_component");

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script_component& script = storage->registry.get<script_component>(entity);
    script_env->destroy_object(script.script_object_id);
  }

}  // namespace other