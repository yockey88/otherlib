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
#include "object/camera_component.hpp"
#include "object/object_serialization_data.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene_network_context.hpp"

#include "entt/entity/fwd.hpp"
#include "scene_storage.hpp"
#include "sol/table.hpp"

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

    sol::table scene_table = storage->sandbox["__other_native"]["__native_scene"];
    sol::table scene_interface_table = storage->sandbox["__other_native"]["__scene_interface"];
    sol::table scene_obj_interface_table = storage->sandbox["__other_native"]["__scene_object_interface"];

    scene_table["__native_pointer"] = this;
    scene_table.set_function("create_scene_object", [this]() -> natural_t { return create_object().id; });
    scene_table["name"] = name;
    scene_table["id"] = id;
    scene_table["set_clear_color"] = [this](glm::vec4 color) {
      this->storage->clear_color = color;
    };

    scene_table["create_scene_object"] = sol::overload(
      [this]() -> natural_t {
        scene_object& new_obj = this->create_object();
        return new_obj.id;
      },
      [this](const std::string& name) -> natural_t {
        scene_object& new_obj = this->create_object(name);
        return new_obj.id;
      },
      [this](const std::string& name, const glm::vec3& world_position) -> natural_t {
        scene_object& new_obj = this->create_object(name, world_position);
        return new_obj.id;
      }
    );
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
    other.do_scene_unbinding();
    *this = std::move(other);
  }

  scene& scene::operator=(scene&& other) {
    if (this == &other) {
      return *this;
    }
    other.do_scene_unbinding();

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

  void scene::run_lua_file(const filepath& script_path) {
    // Load and execute the Lua script
    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "Failed to retrieve scripting environment.");

    lua_sandbox& sandbox = storage->sandbox;
    lua_host& lua = scripting_env->get_lua_host();

    CORE_LOG_DEBUG("Attempting to retrieve scene table from Lua file: {}", script_path.string());
    sol::table scene_table = sandbox.try_load_table(&lua, script_path);
    sol::table objects_table;

    bool scene_valid = scene_table.valid();
    if (!scene_valid) {
      CORE_LOG_ERROR("Failed to load scene-table from Lua file: {}", script_path.string());
      CORE_LOG_WARN(" - Make sure your Lua scene file returns a global table!");
    } else {
      objects_table = scene_table["Objects"];
      scene_valid = objects_table.valid();
      if (!scene_valid) {
        CORE_LOG_WARN("Scene Lua file '{}' does not contain a valid 'Objects' table.", script_path.string());
      } else {
      }
    }

    if (sandbox["OnSceneLoad"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneLoad' from Lua file: {}", script_path.string());
      sol::protected_function on_scene_load_fn = sandbox["OnSceneLoad"];
      sol::protected_function_result result = on_scene_load_fn(scene_table);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneLoad' from Lua file: {}", script_path.string());
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    if (scene_valid) {
      CORE_LOG_DEBUG("Loading scene objects from Lua file: {}", script_path.string());
      for (auto& obj : objects_table) {
        sol::table obj_table = obj.second.as<sol::table>();
        CORE_LOG_DEBUG(" - loading scene object from Lua table...");

        natural_t id = obj_table["GetId"](obj_table);
        OTHER_ASSERT(has_object(id), "Scene object with ID {} already exists!", id);

        scene_object& scene_obj = get_object(id);
        construct_object_from_lua_table(scene_obj, obj_table);
      }

      CORE_LOG_INFO("Loaded scene '{}' from Lua file '{}'.", name, script_path.string());
    }
  }

  void scene::reset() {
  }

  scene scene::create_scene(const std::string& name) {
    return scene(name);
  }

  scene scene::load_scene(const filepath& scene_path) {
    std::string ext = scene_path.extension().string();
    switch (FNV(ext)) {
      case FNV(".lua"): {
        PROFILE_SECTION("scene::load_scene_lua");
        return load_from_lua_file(scene_path);
      }
      default:
        CORE_LOG_ERROR("Unsupported scene file extension '{}'", ext);
        return scene();
    }
  }

  void scene::fixed_update(double delta_time) {
    PROFILE_SECTION("scene::fixed_update");

    if (storage->sandbox["OnSceneFixedUpdate"].valid()) {
      sol::protected_function on_fixed_update_fn = storage->sandbox["OnSceneFixedUpdate"];
      sol::protected_function_result result = on_fixed_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnFixedUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      // comp.fixed_update(delta_time);
    });
  }

  void scene::update(double delta_time) {
    PROFILE_SECTION("scene::update");

    if (storage->sandbox["OnSceneUpdate"].valid()) {
      sol::protected_function on_update_fn = storage->sandbox["OnSceneUpdate"];
      sol::protected_function_result result = on_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    // check_synchronization_updates();

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

    if (storage->sandbox["OnSceneLateUpdate"].valid()) {
      sol::protected_function on_late_update_fn = storage->sandbox["OnSceneLateUpdate"];
      sol::protected_function_result result = on_late_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnLateUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

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

  scene_object& scene::create_object() {
    PROFILE_SECTION("scene::create_object_default");
    return storage->tree.create_object("Scene Object", glm::vec3(0.f), nullptr);
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

  bool scene::has_object(const std::string_view name) const {
    PROFILE_SECTION("scene::has_object_by_id");
    auto* node = storage->tree.find_object_by_name(name);
    return node != nullptr;
  }

  bool scene::has_object(natural_t id) const {
    PROFILE_SECTION("scene::has_object_by_id");
    scene_tree::node* node = storage->tree.node_at(id);
    return node != nullptr && node->object != nullptr;
  }

  scene_object& scene::get_object(const std::string_view name) {
    PROFILE_SECTION("scene::get_object_by_name");
    auto* node = storage->tree.find_object_by_name(name);
    OTHER_ASSERT(node != nullptr, "Scene object with name '{}' not found in scene '{}'.", name, this->name);
    return *node;
  }

  const scene_object& scene::get_object(const std::string_view name) const {
    PROFILE_SECTION("scene::get_object_by_name_const");
    auto* node = storage->tree.find_object_by_name(name);
    OTHER_ASSERT(node != nullptr, "Scene object with name '{}' not found in scene '{}'.", name, this->name);
    return *node;
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

  render_data scene::prepare_render_data(scope<asset_handler>& asset_handler) const {
    PROFILE_SECTION("scene::prepare_render_data");

    render_data data;
    data.clear_color = storage->clear_color;

    const camera* primary_camera = nullptr;
    storage->registry.view<object_handle, camera_component>().each([&](const object_handle& handle, const camera_component& cam) {
      if (object_has_tag(handle.id, "main-camera")) {
        primary_camera = &cam.camera;
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

    storage->registry.view<object_handle, render_component>().each([&](const object_handle& handle, render_component& render) {
      if (!render.visible) {
        return;
      }

      bool is_loaded = asset_handler->get_asset_state(render.model_asset_id) == asset_state::LOADED;
      if (!is_loaded) {
        return;
      }

      if (render.obj_model.source == nullptr) {
        natural_t hash = asset_handler->get_asset_hash(render.model_asset_id);
        ref<model_source> model_src = subsystem<renderer_backend>::get()->get_model_source(hash);
        OTHER_ASSERT(model_src != nullptr, "Model source is null for asset ID {}", render.model_asset_id);

        render.obj_model = model_src->produce_model(std::format("{}-model", handle.object->name), render.submesh_indices);
      }

      model* draw_model = &render.obj_model;
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

  void scene::add_component_by_name(scene_object* object, const std::string_view component_name) {
    OTHER_ASSERT(object != nullptr, "Cannot add component to a null scene object.");

    auto* type_data = subsystem<type_database>::get()->get_reflection_data(component_name);
    if (type_data == nullptr) {
      CORE_LOG_ERROR("Component type '{}' not found in type database.", component_name);
      return;
    }

    auto itr = storage->reflection_data.component_types.find(type_data->type_hash);
    if (itr == storage->reflection_data.component_types.end()) {
      CORE_LOG_ERROR("Component type '{}' not registered in scene reflection data.", component_name);
      return;
    }

    if (!itr->second.add_fn) {
      CORE_LOG_ERROR("Component type '{}' does not have a valid add function.", component_name);
      return;
    }
    if (!itr->second.has_fn) {
      CORE_LOG_ERROR("Component type '{}' does not have a valid has function.", component_name);
      return;
    }

    if (itr->second.has_fn(storage->registry, object->registry_id, storage->sandbox, itr->second.custom_registration.has_fn)) {
      CORE_LOG_WARN("Scene object '{}' already has component of type '{}'.", object->name, component_name);
      return;
    }
    itr->second.add_fn(storage->registry, object->registry_id, storage->sandbox, itr->second.custom_registration.add_fn);
  }

  void scene::remove_component_by_name(scene_object* object, const std::string_view component_name) {
    OTHER_ASSERT(object != nullptr, "Cannot remove component from a null scene object.");

    auto* type_data = subsystem<type_database>::get()->get_reflection_data(component_name);
    if (type_data == nullptr) {
      CORE_LOG_ERROR("Component type '{}' not found in type database.", component_name);
      return;
    }

    auto itr = storage->reflection_data.component_types.find(type_data->type_hash);
    if (itr == storage->reflection_data.component_types.end()) {
      CORE_LOG_ERROR("Component type '{}' not registered in scene reflection data.", component_name);
      return;
    }

    if (!itr->second.remove_fn) {
      CORE_LOG_ERROR("Component type '{}' does not have a valid remove function.", component_name);
      return;
    }
    if (!itr->second.has_fn) {
      CORE_LOG_ERROR("Component type '{}' does not have a valid has function.", component_name);
      return;
    }

    if (!itr->second.has_fn(storage->registry, object->registry_id, storage->sandbox, itr->second.custom_registration.has_fn)) {
      CORE_LOG_WARN("Scene object '{}' does not have component of type '{}'.", object->name, component_name);
      return;
    }
    itr->second.remove_fn(storage->registry, object->registry_id, storage->sandbox, itr->second.custom_registration.remove_fn);
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

  void scene::connect_remote_session(integer_t session_id) {
    PROFILE_SECTION("scene::connect_remote_session");

    scene_object& root = root_object();
    if (!has_component<scene_network_context>(&root)) {
      add_component<scene_network_context>(&root);
    }

    auto* net_ctx = get_component<scene_network_context>(&root);
    net_ctx->add_remote_session(session_id);
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
                                                   orthonormal_basis(glm::vec3(0, 1, 0)),
                                                   world_position,
                                                   glm::vec3(1, 1, 1),
                                                   glm::quat(1, 0, 0, 0),
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

  scene scene::load_from_lua_file(const filepath& scene_path) {
    CORE_LOG_DEBUG("Loading scene from Lua file: {}", scene_path.string());
    scene new_scene = scene(scene_path.filename().stem().string());
    new_scene.run_lua_file(scene_path);
    return new_scene;
  }

  void scene::construct_object_from_lua_table(scene_object& scene_obj, sol::table& obj_table) {
    CORE_LOG_DEBUG("Constructing scene object '{}' from Lua table.", scene_obj.name);
    const auto transform_table = obj_table["Transform"];
    const auto scripts_table = obj_table["Scripts"];
    OTHER_ASSERT(transform_table.valid(), "No Transform table found in the Lua object table for object w/ id {}", scene_obj.id);
    OTHER_ASSERT(scripts_table.valid(), "No scripts found in the Lua object table for object w/ id {}", scene_obj.id);

    transform obj_transform = get_transform(&scene_obj);

    auto position = transform_table["local_position"];
    auto rotation = transform_table["local_rotation_quat"];
    auto scale = transform_table["local_scale"];

    if (position.valid()) {
      obj_transform.local_position = glm::vec3{ position["x"].get_or(0.0f), position["y"].get_or(0.0f), position["z"].get_or(0.0f) };
    }
    if (rotation.valid()) {
      obj_transform.local_rotation_quat = glm::quat{ rotation["w"].get_or(1.0f), rotation["x"].get_or(0.0f), rotation["y"].get_or(0.0f), rotation["z"].get_or(0.0f) };
    }
    if (scale.valid()) {
      obj_transform.local_scale = glm::vec3{ scale["x"].get_or(1.0f), scale["y"].get_or(1.0f), scale["z"].get_or(1.0f) };
    }

    set_transform(&scene_obj, obj_transform);

    sol::table dotnet = scripts_table[".NET"];
    // opt<sol::table> lua_scripts = scripts_table["Lua"];

    script_component* script_comp = get_component<script_component>(&scene_obj);
    OTHER_ASSERT(script_comp != nullptr, "Failed to retrieve script component for object w/ id {}", scene_obj.id);

    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "Failed to retrieve scripting environment");

    if (dotnet.valid()) {
      CORE_LOG_DEBUG(" - attaching .NET scripts to script object ID {}", script_comp->script_object_id);
      for (auto& item : dotnet) {
        std::string script_name = item.first.as<std::string>();
        CORE_LOG_DEBUG("[.NET Type Description]: {}", script_name);

        sol::table script_data = item.second.as<sol::table>();
        for (auto& field_item : script_data) {
          CORE_LOG_DEBUG("[.NET Type Data Entry]: {}", field_item.first.as<std::string>());
        }

        opt<std::string> class_name = script_data["ClassName"];
        if (!class_name.has_value()) {
          CORE_LOG_WARN(" - .NET script '{}' for object ID {} does not specify a ClassName.", script_name, script_comp->script_object_id);
          continue;
        }

        scripting_env->attach_dotnet_object(script_comp->script_object_id, class_name.value());
        script_object* obj = scripting_env->get_object(script_comp->script_object_id);
        OTHER_ASSERT(obj != nullptr, "Failed to retrieve .NET script object after attachment for object ID {}", script_comp->script_object_id);

        if (obj->dotnet_object == nullptr) {
          CORE_LOG_ERROR(" - .NET script object '{}' for object ID {} has null dotnet_object after attachment.", script_name, script_comp->script_object_id);
          continue;
        }

        CORE_LOG_DEBUG(" - attaching serialized .NET object '{}' to script object ID {}", script_name, script_comp->script_object_id);
        sol::table fields_table = script_data["Fields"];

        for (auto& field_item : fields_table) {
          std::string field_name = field_item.first.as<std::string>();
          if (field_name.contains("k__BackingField") || field_name.starts_with("<") ||
              /// not sure what this one is but one of the type contains a mysterious generated 'value__' field (enums?? what does it mean?)
              field_name == "value__") {
            continue;
          }

          if (obj->dotnet_object->get_dotnet_field(field_name) == nullptr) {
            CORE_LOG_WARN(" - .NET script '{}' for object ID {} does not have field '{}' defined in the class.", script_name, script_comp->script_object_id, field_name);
            continue;
          }

          auto* dn_field = obj->dotnet_object->get_dotnet_field(field_name);
          OTHER_ASSERT(dn_field != nullptr, " - .NET script '{}' for object ID {} does not have field '{}' defined in the class.", script_name, script_comp->script_object_id, field_name);

          sol::table field_value = field_item.second;
          value_type val_type = field_value["Type"];
          if (val_type == value_type::EMPTY_TYPE) {
            CORE_LOG_ERROR("   - field '{}' on .NET script '{}' for object ID {} has EMPTY_TYPE, skipping.", field_name, script_name, script_comp->script_object_id);
            continue;
          }
          if (val_type != dn_field->get_type()) {
            CORE_LOG_ERROR("   - field '{}' on .NET script '{}' for object ID {} has mismatched type (Lua: {}, .NET: {}), skipping.", field_name, script_name, script_comp->script_object_id, val_type, dn_field->get_type());
            continue;
          }

          CORE_LOG_DEBUG("Writing Field '{}' on .NET script '{}' for object ID {}", field_name, script_name, script_comp->script_object_id);
          CORE_LOG_DEBUG("   - field value type: {}", val_type);

          if (dn_field->is_property()) {
          } else {
          }

          CORE_LOG_DEBUG(" - .NET script '{}' for object ID {} has field '{}' to set. (sol type = {})", script_name, script_comp->script_object_id, field_name, field_value.get_type());

          value val;
          switch (val_type) {
            case value_type::CHAR: val = value(field_value["Value"].get<char>()); break;
            case value_type::OEBOOL: val = value(field_value["Value"].get<bool>()); break;
            case value_type::INT8: val = value(field_value["Value"].get<int8_t>()); break;
            case value_type::UINT8: val = value(field_value["Value"].get<uint8_t>()); break;
            case value_type::INT16: val = value(field_value["Value"].get<int16_t>()); break;
            case value_type::UINT16: val = value(field_value["Value"].get<uint16_t>()); break;
            case value_type::INT32: val = value(field_value["Value"].get<int32_t>()); break;
            case value_type::UINT32: val = value(field_value["Value"].get<uint32_t>()); break;
            case value_type::INT64: val = value(field_value["Value"].get<int64_t>()); break;
            case value_type::UINT64: val = value(field_value["Value"].get<uint64_t>()); break;
            case value_type::FLOAT: val = value(field_value["Value"].get<float>()); break;
            case value_type::DOUBLE: val = value(field_value["Value"].get<double>()); break;
            case value_type::STRING: val = value(field_value["Value"].get<std::string>()); break;
            case value_type::VEC2: {
              sol::table vec_table = field_value["Value"];
              glm::vec2 vec_val = glm::vec2{ vec_table["x"].get_or(0.0f), vec_table["y"].get_or(0.0f) };
              val = value(vec_val);
            } break;
            case value_type::VEC3: {
              sol::table vec_table = field_value["Value"];
              glm::vec3 vec_val = glm::vec3{ vec_table["x"].get_or(0.0f), vec_table["y"].get_or(0.0f), vec_table["z"].get_or(0.0f) };
              val = value(vec_val);
            } break;
            case value_type::VEC4: {
              sol::table vec_table = field_value["Value"];
              glm::vec4 vec_val = glm::vec4{ vec_table["x"].get_or(0.0f), vec_table["y"].get_or(0.0f), vec_table["z"].get_or(0.0f), vec_table["w"].get_or(0.0f) };
              val = value(vec_val);
            } break;
            default: break;
          }

          if (val.type() == value_type::EMPTY_TYPE) {
            CORE_LOG_ERROR("   - could not convert field '{}' value to valid .NET value for script '{}' on object ID {}", field_name, script_name, script_comp->script_object_id);
            continue;
          }
          obj->dotnet_object->set_field(field_name, val);
        }
      }
    }

    // if (lua_scripts.has_value() && lua_scripts->valid()) {
    //   CORE_LOG_DEBUG(" - attaching Lua scripts to script object ID {}", script_comp->script_object_id);
    //   // for (auto& item : lua_scripts) {
    //   //   std::string script_name = item.first.as<std::string>();
    //   //   // sol::table script_data = item.second.as<sol::table>();

    //   //   // Load and attach the Lua script to the script component
    //   //   subsystem<scripting_environment>::get()->attach_lua_script(script_comp->script_object_id, script_name);
    //   // }
    // }
  }

}  // namespace other