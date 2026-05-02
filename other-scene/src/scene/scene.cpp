/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include <cstdint>
#include <ranges>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "math/matrix.hpp"

#include "model/model.hpp"
#include "model/skeleton.hpp"
#include "renderer/camera.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/renderer.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_controller.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/object_serialization_data.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene_network_context.hpp"
#include "scene/scene_storage.hpp"

#include "entt/entity/fwd.hpp"
#include "glm/fwd.hpp"
#include "sol/table.hpp"

namespace other {

  void scene::scene_first_construction_initialization() {
    storage = make_scene_storage(this);
  }

  void scene::do_final_scene_destruction_cleanup() {
    clear_storage(storage);
    storage = nullptr;
  }

  void scene::do_scene_binding() {
    storage->registry.on_construct<script_component>().connect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().connect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().connect<&scene::on_destroy_script_component>(this);

    storage->registry.on_construct<physics_component>().connect<&scene::on_create_physics_component>(this);
    // storage->registry.on_update<physics_component>().connect<&scene::on_update_physics_component>(this);
    storage->registry.on_destroy<physics_component>().connect<&scene::on_destroy_physics_component>(this);

    opt<sol::table> native_table = storage->sandbox["__other_native"];
    if (native_table.has_value() && native_table->valid()) {
      sol::table scene_table = storage->sandbox["__other_native"]["__native_scene"];
      sol::table scene_interface_table = storage->sandbox["__other_native"]["__scene_interface"];

      scene_table["__native_pointer"] = this;
      scene_table.set_function(
        "create_scene_object",
        sol::overload(
          [this](const std::string& name) -> natural_t { return this->create_object(name).id; },
          [this](const std::string& name, const glm::vec3& world_position) -> natural_t { return this->create_object(name, world_position).id; }
        )
      );

      scene_table["name"] = name;
      scene_table["id"] = id;
      scene_table["set_clear_color"] = [this](glm::vec4 color) {
        this->storage->clear_color = color;
      };
    } else {
      CORE_LOG_WARN("Scene native binding table '__other_native' is invalid.");
    }
  }

  void scene::do_scene_unbinding() {
    OTHER_ASSERT(storage != nullptr, "Scene storage is not initialized.");

    storage->registry.on_construct<physics_component>().disconnect<&scene::on_create_physics_component>(this);
    // storage->registry.on_update<physics_component>().disconnect<&scene::on_update_physics_component>(this);
    storage->registry.on_destroy<physics_component>().disconnect<&scene::on_destroy_physics_component>(this);

    storage->registry.on_construct<script_component>().disconnect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().disconnect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().disconnect<&scene::on_destroy_script_component>(this);
  }

  scene::scene() {
    scene_first_construction_initialization();
    do_scene_binding();

    // Create the root object
    scene_object& root = storage->tree.root_object();
    register_object(&root, "Root", glm::vec3(0.0f));
    object_handle* tag = get_component<object_handle>(&root);
    OTHER_ASSERT(tag != nullptr, "Failed to retrieve object handle component for root scene object.");

    storage->scene_root_entity = entt::entity(root.registry_id);
  }

  scene::scene(const std::string_view name) {
    static natural_t next_id = 1;
    this->name = name;
    this->id = next_id++;

    scene_first_construction_initialization();
    do_scene_binding();

    // Create the root object
    scene_object& root = storage->tree.root_object();
    register_object(&root, "Root", glm::vec3(0.0f));
    object_handle* tag = get_component<object_handle>(&root);
    OTHER_ASSERT(tag != nullptr, "Failed to retrieve object handle component for root scene object.");

    storage->scene_root_entity = entt::entity(root.registry_id);
  }

  scene::scene(scene&& other) {
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

  void scene::run_script_file() {
    if (!script_path.has_value() || script_loaded) {
      return;
    }

    CORE_LOG_DEBUG("Running Lua script file '{}' in scene '{}'.", script_path->string(), name);
    // Load and execute the Lua script
    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "Failed to retrieve scripting environment.");

    lua_sandbox& sandbox = storage->sandbox;
    lua_host& lua = scripting_env->get_lua_host();
    sol::table scene_table = sandbox.try_load_table(&lua, *script_path);

    bool scene_valid = scene_table.valid();
    sol::table objects_table;
    if (scene_valid) {
      objects_table = scene_table["Objects"];
      scene_valid = objects_table.valid();
      if (!scene_valid) {
        CORE_LOG_WARN("Scene Lua file '{}' does not contain a valid 'Objects' table.", script_path->string());
      } else {
      }
    }

    if (sandbox["OnSceneLoad"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneLoad' from Lua file: {}", script_path->string());
      sol::protected_function on_scene_load_fn = sandbox["OnSceneLoad"];
      sol::protected_function_result result = on_scene_load_fn(scene_table);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneLoad' from Lua file: {}", script_path->string());
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    if (scene_valid) {
      CORE_LOG_DEBUG("Loading scene objects from Lua file: {}", script_path->string());
      for (auto& obj : objects_table) {
        sol::table obj_table = obj.second.as<sol::table>();
        natural_t id = obj_table["GetId"](obj_table);
        OTHER_ASSERT(has_object(id), "Scene object with ID {} already exists!", id);

        scene_object& scene_obj = get_object(id);
        construct_object_from_lua_table(scene_obj, obj_table);
      }

      CORE_LOG_INFO("Loaded scene '{}' from Lua file '{}'.", name, script_path->string());
      script_loaded = true;
    } else {
      CORE_LOG_ERROR("Failed to load scene '{}' from Lua file '{}'.", name, script_path->string());
    }
  }

  scene scene::create_scene(const std::string& name) {
    return scene(name);
  }

  void scene::play() {
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot play scene because storage is not initialized.");
      return;
    }

    /// store initial state for reset

    CORE_LOG_INFO("Starting scene '{}'", name);
    playing = true;
    storage->physics->start_simulation();

    storage->registry.view<script_component>().each([](entt::entity entity, script_component& comp) {
      comp.scene_start();
    });
  }

  void scene::pause() {
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot pause scene because storage is not initialized.");
      return;
    }

    storage->physics->stop_simulation();
    playing = false;
  }

  void scene::stop() {
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot stop scene because storage is not initialized.");
      return;
    }

    CORE_LOG_INFO("Stopping scene '{}'", name);
    pause();

    storage->registry.view<script_component>().each([](entt::entity entity, script_component& comp) {
      comp.scene_stop();
    });

    reset();
    /// reset initial state
  }

  void scene::reset() {
    /// restore initial state
  }

  void scene::enable_physics_debug_rendering() {
    debug_physics_rendering_enabled = true;
  }

  void scene::disable_physics_debug_rendering() {
    debug_physics_rendering_enabled = false;
  }

  void scene::fixed_update(double delta_time) {
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::fixed_update");

    if (storage->physics != nullptr) {
      storage->physics->step_simulation(delta_time);
      storage->registry.view<physics_component, transform>().each([](entt::entity entity, physics_component& phys_comp, transform& trans) {
        physics_body* body = phys_comp.body;
        if (body == nullptr || !body->active) {
          return;
        }
        glm::vec3 temp_scale;
        decompose_mat4(phys_comp.body->interpolated_transform, trans.local_position, trans.local_rotation_quat, temp_scale);
      });
    }

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.fixed_update(delta_time);
    });

    if (sol::protected_function on_fixed_update_fn = storage->sandbox["OnSceneFixedUpdate"]; on_fixed_update_fn.valid()) {
      sol::protected_function_result result = on_fixed_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnFixedUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  void scene::update(double delta_time) {
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::update");

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
      comp.update(delta_time);
    });

    if (storage->sandbox["OnSceneUpdate"].valid()) {
      sol::protected_function on_update_fn = storage->sandbox["OnSceneUpdate"];
      sol::protected_function_result result = on_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  void scene::late_update(double delta_time) {
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::late_update");

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.late_update(delta_time);
    });

    if (storage->sandbox["OnSceneLateUpdate"].valid()) {
      sol::protected_function on_late_update_fn = storage->sandbox["OnSceneLateUpdate"];
      sol::protected_function_result result = on_late_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnLateUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  scene_object& scene::root_object() {
    PROFILE_SECTION("scene::get_root_object");

    scene_tree::node* root_node = storage->tree.node_at(0);
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene storage->tree.");
    OTHER_ASSERT(root_node->object != nullptr, "Root node object is null.");

    return *root_node->object;
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

  std::vector<scene_object*> scene::get_children(natural_t id) {
    PROFILE_SECTION("scene::get_children");

    const scene_tree::node* node = storage->tree.node_at(id);
    if (node == nullptr) {
      CORE_LOG_ERROR("Node with the given ID does not exist in the scene storage->tree.");
      return {};
    }

    std::vector<scene_object*> children;
    for (const scene_tree::node* child : node->children) {
      if (child != nullptr && child->object != nullptr) {
        children.push_back(child->object);
      }
    }
    return children;
  }

  std::vector<scene_object*> scene::get_children(const scene_object* object) {
    PROFILE_SECTION("scene::get_children");

    if (object == nullptr) {
      return {};
    }
    return get_children(object->id);
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

  scene_object* scene::find_object(const std::string_view name) const {
    scene_object* n = storage->tree.find_object_by_name(name);
    return n != nullptr ? n : nullptr;
  }

  scene_object* scene::find_object(natural_t id) const {
    scene_tree::node* node = storage->tree.node_at(id);
    return (node != nullptr && node->object != nullptr) ? node->object : nullptr;
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

  render_data scene::prepare_render_data(const glm::ivec2 window_size, scope<asset_handler>& asset_handler) const {
    PROFILE_SECTION("scene::prepare_render_data");

    render_data data;
    data.clear_color = storage->clear_color;

    camera* primary_camera = nullptr;
    storage->registry.view<object_handle, camera_component>().each([&](const object_handle& handle, camera_component& cam) {
      if (primary_camera == nullptr && object_has_tag(handle.id, "main-camera")) {
        cam.camera.calculate_matrices(window_size);
        primary_camera = &cam.camera;
      }
    });

    if (primary_camera != nullptr) {
      /// \todo fix this const cast
      data.primary_camera = primary_camera;
    }

    storage->registry.view<object_handle, light_component>().each([&](const object_handle& handle, const light_component& light) {
      std::ranges::copy(light.directional_lights, std::back_inserter(data.ambient_lights));
      std::ranges::copy(light.point_lights, std::back_inserter(data.point_lights));

      if (object_has_tag(handle.id, "scene-ambient-light")) {
        data.scene_ambient_light = &light.directional_lights[0];
      }
    });

    storage->registry.view<object_handle, render_component>().each([&](const object_handle& handle, render_component& render) {
      if (!render.visible) {
        return;
      }

      /**
       * \todo currently @ref render_component::model_asset_id stores the model source id since models are not individually stored in the asset handler
       *          but we need to make it hold individual model asset IDs and do the same type of logic below except without the model source intermediary
       **/

      bool changed = render.last_model_asset_id != render.model_asset_id;
      if (changed) {
        if (!asset_handler->asset_exists(render.model_asset_id)) {
          CORE_LOG_ERROR("Render component model asset ID {} does not exist for object ID {}.", render.model_asset_id, handle.id);
          // avoids repeated failed lookups and objects don't disappear from scene
          render.model_asset_id = render.last_model_asset_id;
        } else {
          CORE_LOG_INFO("Render component model asset ID changed for object ID {}. New asset ID: {}", handle.id, render.model_asset_id);
        }
      }

      /// either they are the same or we already validated the change
      const bool is_loaded = asset_handler->asset_loaded(render.model_asset_id);
      if (!is_loaded) {
        return;
      }

      natural_t hash = asset_handler->get_asset_hash(render.model_asset_id);
      OTHER_ASSERT(hash != 0, "Asset hash is 0 for asset ID {}.", render.model_asset_id);

      /// handle the case this is first load of the model asset ID/a change for this render component
      ///  and we need to produce the model
      if (render.obj_model.source == nullptr) {
        CORE_LOG_INFO("Looking up model source for asset ID {}", render.model_asset_id);

        /// we check the asset exists and is loaded so this can not ever be null
        ref<model_source> model_src = subsystem<renderer_backend>::get()->get_model_source(hash);
        OTHER_ASSERT(model_src != nullptr, "Model source is null for asset ID {}", render.model_asset_id);

        render.obj_model = model_src->produce_model();
      }

      model* draw_model = &render.obj_model;
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

      render.last_model_asset_id = render.model_asset_id;
    });

    if (debug_physics_rendering_enabled && storage->physics != nullptr) {
      physics_api::physics_render_debug_data debug_data = storage->physics->get_debug_render_data();

      auto lines_w_colors = std::views::zip(debug_data.debug_lines, debug_data.debug_line_colors);
      data.debug_data.debug_lines.append_range(lines_w_colors | std::views::transform([](const std::pair<physics_api::line, glm::vec4>& pair) {
                                                 return debug_line{
                                                   .start = pair.first.start,
                                                   .end = pair.first.end,
                                                   .color = pair.second,
                                                 };
                                               }));

      auto triangles_w_colors = std::views::zip(debug_data.debug_triangles, debug_data.debug_triangle_colors);
      data.debug_data.debug_triangles.append_range(triangles_w_colors | std::views::transform([](const std::pair<physics_api::triangle, glm::vec4>& pair) {
                                                     return debug_triangle{
                                                       .v0 = pair.first.v0,
                                                       .v1 = pair.first.v1,
                                                       .v2 = pair.first.v2,
                                                       .color = pair.second,
                                                     };
                                                   }));
    }

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

  void scene::remove_object_tag(natural_t id, const std::string_view tag) {
    PROFILE_SECTION("scene::remove_object_tag");
    scene_tree::node* n = storage->tree.node_at(id);
    OTHER_ASSERT(n != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    n->tags.erase(std::remove_if(n->tags.begin(), n->tags.end(), [&](const object_tag& t) { return t.name == tag; }), n->tags.end());
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

    auto* net_ctx = get_component<scene_network_context>(&root);
    OTHER_ASSERT(net_ctx != nullptr, "Scene network context component is not present on the root scene object.");

    net_ctx->add_remote_session(session_id);
  }

  scene::object_handle::operator scene_object*() const {
    OTHER_ASSERT(object != nullptr, "Object handle is null, cannot convert to scene_object*.");
    return object;
  }

  bool scene::object_handle::operator==(const object_handle& other) const {
    return id == other.id && object == other.object;
  }

  scene_object* scene::from_registry_id(entt::entity entity) {
    PROFILE_SECTION("scene::from_registry_id");

    object_handle* handle = storage->registry.try_get<object_handle>(entity);
    if (handle == nullptr) {
      CORE_LOG_ERROR("Object handle not found for entity {}", (natural_t)entity);
      return nullptr;
    }
    return handle->object;
  }

  void scene::register_object(scene_object* object, const std::string& name, const glm::vec3& world_position) {
    PROFILE_SECTION("scene::register_object");

    OTHER_ASSERT(name.size() > 0, "Scene object name cannot be empty.");
    OTHER_ASSERT(object != nullptr, "Cannot register a null scene object.");

    CORE_LOG_DEBUG(" - registering scene object '{}' in scene '{}'", name, this->name);
    entt::entity entity = storage->registry.create();
    object->name = name;
    object->registry_id = (uint32_t)entity;

    storage->registry.emplace<object_handle>(entity, object_handle{ .id = (natural_t)entity, .object = object });
    storage->registry.emplace<component_registry>(entity, component_registry{});
    storage->registry.emplace<transform>(entity, transform{
                                                   orthonormal_basis(glm::vec3(0, 1, 0)),
                                                   world_position,
                                                   glm::vec3(1, 1, 1),
                                                   glm::quat(),
                                                 });
    storage->registry.emplace<script_component>(entity, script_component{ object });

    transform& transf = storage->registry.get<transform>(entity);
    script_component& script = storage->registry.get<script_component>(entity);
    auto& comp_reg = storage->registry.get<component_registry>(entity);
    comp_reg.register_component(transf);
    comp_reg.register_component(script);
  }

  void scene::register_object(scene_object* object, const std::string& name, const transform& transformation) {
    register_object(object, name, transformation.local_position);
    set_transform(object, transformation);
  }

  void scene::unregister_object(scene_object* object) {
    PROFILE_SECTION("scene::unregister_object");
    // auto& comb_reg = get_component<component_registry>(object);
    // comp_reg.unregister_all();
  }

  void scene::on_create_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_update_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_destroy_render_component(const entt::registry&, const entt::entity entity) {
  }

  void scene::on_create_script_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_create_script_component");

    script_component* script = storage->registry.try_get<script_component>(entity);
    if (script == nullptr) {
      CORE_LOG_ERROR("Script component not found for entity {}", (natural_t)entity);
      return;
    }
    OTHER_ASSERT(script->object != nullptr, "Scene object reference in script component is null for entity {}", (natural_t)entity);

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script->script_object_id = script_env->create_object(script->object->name);
    CORE_LOG_DEBUG("Created script object with ID {} for scene object '{}' [ID: {}] (entity {})", script->script_object_id, script->object->name, script->object->id, (natural_t)entity);

    script_object* object = script_env->get_object(script->script_object_id);
    OTHER_ASSERT(object != nullptr, "Failed to retrieve script object after creation for object ID {}", script->script_object_id);

    script_env->attach_dotnet_object(script->script_object_id, "Other.SceneObject", (void*)script->object);
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

  void scene::on_create_physics_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_create_physics_component");

    physics_component& physics_comp = storage->registry.get<physics_component>(entity);
    object_handle& obj_handle = storage->registry.get<object_handle>(entity);
    physics_comp.settings.world_transform = get_world_transform(&get_object(obj_handle.id));

    OTHER_ASSERT(storage->physics != nullptr, "Scene physics storage is not initialized.");
    physics_comp.body = storage->physics->create_physics_body(physics_comp.settings);
    physics_comp.shape = storage->physics->create_empty_shape(physics_comp.body);
    physics_comp.body->active = true;

    OTHER_ASSERT(physics_comp.body != nullptr, "Failed to create physics body for entity {}", (natural_t)entity);
  }

  // void scene::on_update_physics_component(const entt::registry&, const entt::entity entity) {}

  void scene::on_destroy_physics_component(const entt::registry&, const entt::entity entity) {
    PROFILE_SECTION("scene::on_destroy_physics_component");

    physics_component& physics_comp = storage->registry.get<physics_component>(entity);

    OTHER_ASSERT(storage->physics != nullptr, "Scene physics storage is not initialized.");
    storage->physics->destroy_physics_shape(physics_comp.shape);
    storage->physics->destroy_physics_body(physics_comp.body);
    physics_comp.shape = nullptr;
    physics_comp.body = nullptr;
  }

  void scene::construct_object_from_lua_table(scene_object& scene_obj, sol::table& obj_table) {
    PROFILE_SECTION("scene::construct_object_from_lua_table");

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

    opt<sol::table> dotnet = obj_table["DotNetClasses"];
    // opt<sol::table> lua_scripts = scripts_table["Lua"];
    if (dotnet.has_value() && dotnet->valid()) {
      script_component* script_comp = get_component<script_component>(&scene_obj);
      OTHER_ASSERT(script_comp != nullptr, "Failed to retrieve script component for object w/ id {}", scene_obj.id);

      auto* scripting_env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(scripting_env != nullptr, "Failed to retrieve scripting environment");

      CORE_LOG_DEBUG(" - attaching .NET scripts to script object ID {} ({} scripts)", script_comp->script_object_id, dotnet->size());
      for (const auto& kv : *dotnet) {
        const auto& script_name = kv.first.as<std::string>();
        if (!kv.second.is<std::string>()) {
          CORE_LOG_WARN(" - .NET script '{}' for object ID {} does not have a valid class name string, skipping.", script_name, scene_obj.id);
          continue;
        }

        const auto& class_name = kv.second.as<std::string>();
        CORE_LOG_DEBUG("   - attaching .NET script '{}' with class name '{}' to script object ID {}", script_name, class_name, script_comp->script_object_id);
        scripting_env->attach_dotnet_object(script_comp->script_object_id, class_name);
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