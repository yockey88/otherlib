/**
 * \file scene/scene.cpp
 **/
#include "scene/scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "math/matrix.hpp"
#include "serialization/scene_serializer.hpp"
#include "thread/thread_safety.hpp"

#include "model/model.hpp"
#include "model/skeleton.hpp"
#include "physics/physics_environment.hpp"
#include "renderer/camera.hpp"
#include "renderer/draw_command.hpp"
#include "renderer/gpu_structs.hpp"
#include "renderer/renderer.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_component.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/physics_joint_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene_network_context.hpp"
#include "scene/scene_storage.hpp"

#include "entt/entity/fwd.hpp"
#include "glm/fwd.hpp"

namespace other {

  void scene::scene_first_construction_initialization() {
    ASSERT_MAIN_THREAD();
    storage = make_scene_storage(this);
  }

  void scene::do_final_scene_destruction_cleanup() {
    ASSERT_MAIN_THREAD();
    clear_storage(storage);
    storage = nullptr;
  }

  void scene::do_scene_binding() {
    ASSERT_MAIN_THREAD();
    storage->registry.on_construct<script_component>().connect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().connect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().connect<&scene::on_destroy_script_component>(this);

    storage->registry.on_construct<physics_component>().connect<&scene::on_create_physics_component>(this);
    // storage->registry.on_update<physics_component>().connect<&scene::on_update_physics_component>(this);
    storage->registry.on_destroy<physics_component>().connect<&scene::on_destroy_physics_component>(this);
  }

  void scene::do_scene_unbinding() {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(storage != nullptr, "Scene storage is not initialized.");

    storage->registry.on_construct<physics_component>().disconnect<&scene::on_create_physics_component>(this);
    // storage->registry.on_update<physics_component>().disconnect<&scene::on_update_physics_component>(this);
    storage->registry.on_destroy<physics_component>().disconnect<&scene::on_destroy_physics_component>(this);

    storage->registry.on_construct<script_component>().disconnect<&scene::on_create_script_component>(this);
    // storage->registry.on_update<script_component>().disconnect<&scene::on_update_script_component>(this);
    storage->registry.on_destroy<script_component>().disconnect<&scene::on_destroy_script_component>(this);
  }

  scene::scene() {
    ASSERT_MAIN_THREAD();
    CORE_LOG_DEBUG("Constructing scene: {}", name);
    scene_first_construction_initialization();
    do_scene_binding();

    // Create the root object
    scene_object& root = create_object(name + ":Root", glm::vec3(0.0f));
    object_handle* tag = get_component<object_handle>(&root);
    OTHER_ASSERT(tag != nullptr, "Failed to retrieve object handle component for root scene object.");

    storage->scene_root_entity = entt::entity(root.registry_id);
  }

  scene::scene(const std::string_view name) {
    ASSERT_MAIN_THREAD();
    CORE_LOG_DEBUG("Constructing scene: {}", name);
    this->name = name;
    this->id = FNV(name);

    scene_first_construction_initialization();
    do_scene_binding();

    // Create the root object
    scene_object& root = create_object(this->name + ":Root", glm::vec3(0.0f));
    object_handle* tag = get_component<object_handle>(&root);
    OTHER_ASSERT(tag != nullptr, "Failed to retrieve object handle component for root scene object.");

    storage->scene_root_entity = entt::entity(root.registry_id);
  }

  scene::scene(scene&& other) {
    ASSERT_MAIN_THREAD();
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
    ASSERT_MAIN_THREAD();
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
    ASSERT_MAIN_THREAD();
    if (storage != nullptr) {
      storage->tree.destroy_all_objects();

      do_scene_unbinding();
      do_final_scene_destruction_cleanup();
    }
  }

  void scene::run_script_file() {
    ASSERT_MAIN_THREAD();
    if (!script_path.has_value() || script_loaded) {
      return;
    }

    if (!std::filesystem::exists(*script_path)) {
      CORE_LOG_ERROR("Scene '{}' hook script '{}' does not exist.", name, script_path->string());
      script_loaded = true;
      return;
    }

    /// the chunk defines behavior hooks (OnSceneLoad/Update/Render/...) in this scene's sandbox
    /// scene CONTENT comes from the scene document, never from lua
    CORE_LOG_DEBUG("Running Lua hook script '{}' in scene '{}'.", script_path->string(), name);
    auto* scripting_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(scripting_env != nullptr, "Failed to retrieve scripting environment.");

    lua_sandbox& sandbox = *storage->sandbox;
    lua_host& lua = scripting_env->get_lua_host();
    (void)sandbox.try_load_table(&lua, *script_path);  /// return value optional and unused

    if (sandbox["OnSceneLoad"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneLoad' from Lua file: {}", script_path->string());
      sol::protected_function on_scene_load_fn = sandbox["OnSceneLoad"];
      sol::protected_function_result result = on_scene_load_fn();
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneLoad' from Lua file: {}", script_path->string());
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    CORE_LOG_DEBUG("Scene '{}' Lua file '{}' executed", name, script_path->string());
    script_loaded = true;
  }

  void scene::set_pending_document(serialization::scene_document&& doc) {
    pending_document = std::move(doc);
  }

  void scene::instantiate_pending_document() {
    ASSERT_MAIN_THREAD();
    if (!pending_document.has_value()) {
      return;
    }

    CORE_LOG_DEBUG("Instantiating scene document into scene '{}' ({} objects).", name, pending_document->objects.size());
    serialization::instantiate_scene(*this, *pending_document, serialization::default_codec_services());

    script_source = pending_document->script;
    if (!script_source.empty()) {
      if (const filepath script_relative = filepath(script_source); script_relative.is_absolute()) {
        script_path = script_relative;
      } else if (source_path.has_value()) {
        script_path = source_path->parent_path() / script_relative;
      } else {
        CORE_LOG_ERROR("Scene '{}' declares hook script '{}' but has no source path to resolve it against.", name, script_source);
      }
    }

    pending_document.reset();
  }

  scene scene::create_scene(const std::string& name) {
    ASSERT_MAIN_THREAD();
    return scene(name);
  }

  void scene::play() {
    ASSERT_MAIN_THREAD();
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot play scene because storage is not initialized.");
      return;
    }

    play_snapshot = capture_snapshot();

    playing = true;
    if (storage->physics != nullptr) {
      revalidate_physics();  /// restored/edited bodies must match their authored settings before simulating
      seed_physics_poses();  /// edit-mode moves happened while physics was frozen
      create_scene_joints();
      storage->physics->start_simulation();
    }

    storage->registry.view<script_component>().each([](entt::entity entity, script_component& comp) {
      comp.scene_start();
    });
  }

  void scene::pause() {
    ASSERT_MAIN_THREAD();
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot pause scene because storage is not initialized.");
      return;
    }

    if (storage->physics != nullptr) {
      storage->physics->stop_simulation();
    }
    playing = false;
  }

  void scene::stop() {
    ASSERT_MAIN_THREAD();
    if (storage == nullptr) {
      CORE_LOG_ERROR("Cannot stop scene because storage is not initialized.");
      return;
    }

    if (storage->physics != nullptr) {
      destroy_scene_joints();  /// welds go before the restore tears their bodies down
    }
    pause();

    storage->registry.view<script_component>().each([](entt::entity entity, script_component& comp) {
      comp.scene_stop();
    });

    /// end of the disable pass: the restore invalidates every runtime id, so the
    ///  surviving managed instances drop their native bindings here and are rebound
    ///  during the rebuild — Awake/Remove never fire on a play-stop cycle. the root
    ///  survives the restore untouched, so its binding stays valid
    if (!play_snapshot.empty()) {
      scene_object& root = root_object();
      storage->registry.view<script_component>().each([&root](entt::entity entity, script_component& comp) {
        if (comp.object == &root) {
          return;
        }
        comp.reset_dotnet_binding();
      });

      preserving_script_objects = true;
      reset();
      preserving_script_objects = false;
    }
  }

  void scene::reset() {
    ASSERT_MAIN_THREAD();
    if (play_snapshot.empty()) {
      return;
    }

    CORE_LOG_DEBUG("Restoring scene '{}' to its pre-play state ({} snapshot bytes).", name, play_snapshot.size());
    /// restore consumes the snapshot — the next play() captures a fresh one
    ostd::vector<uint8_t> snapshot = std::move(play_snapshot);
    play_snapshot.clear();
    restore_snapshot(snapshot);
  }

  ostd::vector<uint8_t> scene::capture_snapshot() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::capture_snapshot");
    return serialization::write_scene_binary(serialization::capture_scene(*this, serialization::default_codec_services()));
  }

  void scene::restore_snapshot(std::span<const uint8_t> snapshot_bytes) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::restore_snapshot");

    serialization::scene_parse_result parsed = serialization::parse_scene_binary(snapshot_bytes);
    if (!parsed.success()) {
      CORE_LOG_ERROR("Cannot restore scene '{}' snapshot: {}", name, parsed.error);
      return;
    }

    destroy_all_non_root_objects();
    serialization::instantiate_scene(*this, *parsed.document, serialization::default_codec_services());

    /// whatever the rebuild did not reclaim was created during play and is not part of
    ///  the restored state — destroying it now is a genuine removal (Remove fires)
    if (!preserved_script_objects.empty()) {
      auto* script_env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");
      for (const integer_t orphan_id : preserved_script_objects) {
        CORE_LOG_DEBUG("Destroying script object with ID {} left unclaimed by the restore.", orphan_id);
        script_env->destroy_object(orphan_id);
      }
      preserved_script_objects.clear();
    }
  }

  void scene::destroy_all_non_root_objects() {
    ASSERT_MAIN_THREAD();
    const natural_t root_id = root_object().id;
    /// copy the child list — destroy mutates it
    const ostd::vector<uint64_t> children = get_children_ids(root_id);
    for (const natural_t child_id : children) {
      destroy_object(child_id);
    }
  }

  ostd::vector<std::string> scene::get_object_tags(natural_t id) const {
    ASSERT_MAIN_THREAD();
    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");

    ostd::vector<std::string> tags = {};
    for (const object_tag& tag : node->tags) {
      tags.push_back(tag.name);
    }
    return tags;
  }

  void scene::enable_physics_debug_rendering() {
    ASSERT_MAIN_THREAD();
    debug_physics_rendering_enabled = true;
  }

  void scene::disable_physics_debug_rendering() {
    ASSERT_MAIN_THREAD();
    debug_physics_rendering_enabled = false;
  }

  void scene::fixed_update(double delta_time) {
    ASSERT_MAIN_THREAD();
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::fixed_update");

    if (storage->physics != nullptr) {
      push_kinematic_targets(delta_time);             /// entity transform -> kinematic sweep target
      storage->physics->step_simulation(delta_time);  /// exactly one fixed step
      step_contacts.clear();
      storage->physics->drain_contacts(step_contacts);
      dispatch_contact_events(step_contacts);         /// scripts hear contacts before their FixedUpdate
      check_joint_breaks(delta_time);
    }

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.fixed_update(delta_time);
    });

    if (sol::protected_function on_fixed_update_fn = (*storage->sandbox)["OnSceneFixedUpdate"]; on_fixed_update_fn.valid()) {
      sol::protected_function_result result = on_fixed_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnFixedUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  void scene::sync_physics_transforms(float alpha) {
    PROFILE_SECTION("scene::sync_physics_transforms");
    storage->physics->interpolate_active_transforms(alpha);

    storage->registry.view<object_handle, physics_component, transform>().each(
      [this](entt::entity entity, object_handle& handle, physics_component& phys_comp, transform& trans) {
        physics_body* body = phys_comp.body;
        if (body == nullptr || !body->active || body->body_type != physics_body::DYNAMIC) {
          return;  /// statics don't move, kinematics are entity-driven
        }

        /// the body pose is world space, the entity stores local TRS under its parent
        glm::mat4 local = body->interpolated_transform;
        if (const scene_object* parent = get_parent(handle.id); parent != nullptr) {
          local = glm::inverse(get_world_transform(parent->id)) * local;
        }

        glm::vec3 unused_scale;  /// physics never writes scale
        decompose_mat4(local, trans.local_position, trans.local_rotation_quat, unused_scale);
      });
  }

  namespace {

    /// positions/indices borrowed from the entity's resolved render model for hull/mesh builds
    struct extracted_geometry {
      ostd::vector<glm::vec3> positions;
      ostd::vector<uint32_t> indices;
      bool valid() const { return !positions.empty(); }
    };

    extracted_geometry extract_render_geometry(const model_data& data) {
      extracted_geometry geo;
      geo.positions.reserve(data.vertices.size());
      for (const vertex& v : data.vertices) {
        geo.positions.push_back(v.position);
      }
      geo.indices.reserve(data.indices.size() * 3);
      for (const index& tri : data.indices) {  /// model indices are whole triangles
        geo.indices.push_back(tri.v0);
        geo.indices.push_back(tri.v1);
        geo.indices.push_back(tri.v2);
      }
      return geo;
    }

  }  // namespace

  void scene::apply_component_shape(entt::entity entity, physics_component& phys_comp) {
    object_handle& handle = storage->registry.get<object_handle>(entity);

    glm::vec3 position, scale;
    glm::quat rotation;
    decompose_mat4(get_world_transform(handle.id), position, rotation, scale);

    /// hull/mesh/fit need the render model; unresolved models defer (revalidation retries)
    const model_data* data = nullptr;
    if (const render_component* rc = storage->registry.try_get<render_component>(entity);
        rc != nullptr && rc->obj_model.source != nullptr) {
      data = &rc->obj_model.source->source_data();
    }

    const physics_shape_desc& desc = phys_comp.settings.shape;

    extracted_geometry geo;
    shape_geometry spans;
    shape_geometry* geo_ptr = nullptr;
    if (shape_needs_geometry(desc)) {
      if (data == nullptr || data->vertices.empty()) {
        return;
      }
      geo = extract_render_geometry(*data);
      spans.positions = geo.positions;
      spans.indices = geo.indices;
      geo_ptr = &spans;
    }

    const bounding_box* fit_bounds = (desc.fit_render_bounds && data != nullptr) ? &data->bounds : nullptr;
    phys_comp.shape = storage->physics->apply_shape(phys_comp.body, desc, scale, geo_ptr, fit_bounds);
  }

  void scene::rebuild_physics_body(entt::entity entity, physics_component& phys_comp) {
    object_handle& handle = storage->registry.get<object_handle>(entity);

    if (phys_comp.shape != nullptr) {
      storage->physics->destroy_physics_shape(phys_comp.shape);
      phys_comp.shape = nullptr;
    }
    if (phys_comp.body != nullptr) {
      storage->physics->destroy_physics_body(phys_comp.body);
    }

    phys_comp.body = storage->physics->create_physics_body(phys_comp.settings, get_world_transform(handle.id));
    OTHER_ASSERT(phys_comp.body != nullptr, "Failed to rebuild physics body for object {}", handle.id);
    phys_comp.body->owner_object_id = handle.id;
    phys_comp.body->active = true;
  }

  void scene::revalidate_physics() {
    PROFILE_SECTION("scene::revalidate_physics");
    storage->registry.view<object_handle, physics_component, transform>().each(
      [this](entt::entity entity, object_handle& handle, physics_component& phys_comp, transform&) {
        if (phys_comp.body == nullptr) {
          return;  /// physics-off profile
        }

        /// authored body diverged from the built body (restore, inspector, C#) -> recreate;
        /// the fresh body starts shapeless and falls through to the shape check below
        if (!(phys_comp.settings == phys_comp.body->applied_settings)) {
          rebuild_physics_body(entity, phys_comp);
        }

        glm::vec3 position, scale;
        glm::quat rotation;
        decompose_mat4(get_world_transform(handle.id), position, rotation, scale);

        const bool never_built = phys_comp.shape == nullptr;
        const bool desc_dirty = !never_built && !(phys_comp.settings.shape == phys_comp.shape->applied);
        const bool scale_dirty = !never_built && glm::length(scale - phys_comp.shape->applied_scale) > 0.0001f;
        if (never_built || desc_dirty || scale_dirty) {
          apply_component_shape(entity, phys_comp);
        }
      });
  }

  void scene::seed_physics_poses() {
    PROFILE_SECTION("scene::seed_physics_poses");
    storage->registry.view<object_handle, physics_component>().each([this](entt::entity entity, object_handle& handle, physics_component& phys_comp) {
      if (phys_comp.body == nullptr) {
        return;
      }
      storage->physics->teleport_body(phys_comp.body, get_world_transform(handle.id));
    });
  }

  void scene::push_kinematic_targets(double step) {
    PROFILE_SECTION("scene::push_kinematic_targets");
    storage->registry.view<object_handle, physics_component>().each([this, step](entt::entity entity, object_handle& handle, physics_component& phys_comp) {
      if (phys_comp.body == nullptr || phys_comp.body->body_type != physics_body::KINEMATIC) {
        return;
      }
      storage->physics->move_kinematic(phys_comp.body, get_world_transform(handle.id), step);
    });
  }

  void scene::dispatch_contact_events(const ostd::vector<contact_event>& events) {
    PROFILE_SECTION("scene::dispatch_contact_events");
    for (const contact_event& ev : events) {
      physics_body* a = storage->physics->body_by_id(ev.body_a);
      physics_body* b = storage->physics->body_by_id(ev.body_b);
      if (a == nullptr || b == nullptr) {
        continue;  /// a side was destroyed the same tick — dropped by contract
      }

      const char* method = nullptr;
      switch (ev.type) {
        case contact_event::kBegin: method = "CollisionEnter"; break;
        case contact_event::kEnd: method = "CollisionExit"; break;
        case contact_event::kTriggerBegin: method = "TriggerEnter"; break;
        case contact_event::kTriggerEnd: method = "TriggerExit"; break;
      }

      /// both sides hear about the other; the normal points away from the receiver
      dispatch_physics_event_to(a->owner_object_id, method, b->owner_object_id, ev.point, ev.normal);
      dispatch_physics_event_to(b->owner_object_id, method, a->owner_object_id, ev.point, -ev.normal);
    }
  }

  void scene::dispatch_physics_event_to(natural_t object_id, const char* method, natural_t other_id,
                                        const glm::vec3& point, const glm::vec3& normal) {
    scene_object* obj = find_object(object_id);
    if (obj == nullptr) {
      return;
    }
    if (script_component* comp = storage->registry.try_get<script_component>(entt::entity(obj->registry_id)); comp != nullptr) {
      comp->dispatch_physics_event(method, other_id, point, normal);
    }
  }

  void scene::create_scene_joints() {
    PROFILE_SECTION("scene::create_scene_joints");
    storage->registry.view<object_handle, physics_joint_component>().each([this](entt::entity entity, object_handle& handle, physics_joint_component& joint_comp) {
      joint_comp.joint_id = -1;
      if (joint_comp.broken) {
        return;
      }

      scene_object* target = find_object(joint_comp.target_object_name);
      if (target == nullptr) {
        CORE_LOG_WARN("Physics joint on '{}' targets unknown object '{}'.", get_object(handle.id).name, joint_comp.target_object_name);
        return;
      }

      physics_component* own = storage->registry.try_get<physics_component>(entity);
      physics_component* other_comp = storage->registry.try_get<physics_component>(entt::entity(target->registry_id));
      if (own == nullptr || own->body == nullptr || other_comp == nullptr || other_comp->body == nullptr) {
        CORE_LOG_WARN("Physics joint between '{}' and '{}' requires physics bodies on both objects.",
                      get_object(handle.id).name, joint_comp.target_object_name);
        return;
      }

      joint_comp.joint_id = storage->physics->create_fixed_joint(own->body, other_comp->body);
    });
  }

  void scene::destroy_scene_joints() {
    storage->registry.view<physics_joint_component>().each([this](entt::entity entity, physics_joint_component& joint_comp) {
      if (joint_comp.joint_id >= 0) {
        storage->physics->destroy_joint(joint_comp.joint_id);
        joint_comp.joint_id = -1;
      }
    });
  }

  void scene::check_joint_breaks(double step) {
    PROFILE_SECTION("scene::check_joint_breaks");
    storage->registry.view<object_handle, physics_joint_component>().each([this, step](entt::entity entity, object_handle& handle, physics_joint_component& joint_comp) {
      if (joint_comp.joint_id < 0 || joint_comp.broken || joint_comp.break_force <= 0.f) {
        return;
      }

      float force = storage->physics->joint_reaction_force(joint_comp.joint_id, step);
      if (force <= joint_comp.break_force) {
        return;
      }

      storage->physics->destroy_joint(joint_comp.joint_id);
      joint_comp.joint_id = -1;
      joint_comp.broken = true;

      dispatch_joint_break_to(handle.id, force);
      if (scene_object* target = find_object(joint_comp.target_object_name); target != nullptr) {
        dispatch_joint_break_to(target->id, force);
      }
    });
  }

  void scene::dispatch_joint_break_to(natural_t object_id, float force) {
    scene_object* obj = find_object(object_id);
    if (obj == nullptr) {
      return;
    }
    if (script_component* comp = storage->registry.try_get<script_component>(entt::entity(obj->registry_id)); comp != nullptr) {
      comp->dispatch_joint_break(force);
    }
  }

  void scene::sync_edit_mode_physics_poses() {
    ASSERT_MAIN_THREAD();
    if (playing || storage->physics == nullptr) {
      return;
    }
    PROFILE_SECTION("scene::sync_edit_mode_physics_poses");

    storage->registry.view<object_handle, physics_component>().each([this](entt::entity entity, object_handle& handle, physics_component& phys_comp) {
      if (phys_comp.body == nullptr) {
        return;
      }
      glm::mat4 world = get_world_transform(handle.id);
      glm::vec3 position, scale;
      glm::quat rotation;
      decompose_mat4(world, position, rotation, scale);

      const bool moved = glm::length(position - phys_comp.body->get_current_position()) > 1e-4f ||
        std::abs(glm::dot(rotation, phys_comp.body->get_current_rotation())) < 1.f - 1e-5f;
      if (moved) {
        storage->physics->teleport_body(phys_comp.body, world);
      }
    });
  }

  namespace {

    /// resolve the component's clip, advance its clock, sample the working pose, build the
    ///  model's palette. runs after every script surface so state scripts set lands in the
    ///  same frame's pose
    void tick_animation(animation_component& anim, render_component& render, double delta_time, scope<asset_handler>& asset_handler) {
      model_source* source = render.obj_model.source;
      if (source == nullptr || source->source_data().skel.empty()) {
        anim.clip = nullptr;
        anim.bound_skeleton = nullptr;
        return;
      }

      /// the render_component validation dance, clip flavored
      /// a bad assignment reverts instead of killing playback
      if (anim.last_animation_asset_id != anim.animation_asset_id && anim.animation_asset_id != 0) {
        if (!asset_handler->asset_exists(anim.animation_asset_id)) {
          CORE_LOG_ERROR("Animation component clip asset ID {} does not exist.", anim.animation_asset_id);
          anim.animation_asset_id = anim.last_animation_asset_id;
        }
      }
      anim.last_animation_asset_id = anim.animation_asset_id;

      const animation_clip* resolved = nullptr;
      if (anim.animation_asset_id != 0) {
        /// standalone .oanim through the backend registry
        /// not-yet-loaded stays in bind pose
        if (asset_handler->asset_loaded(anim.animation_asset_id)) {
          resolved = subsystem<renderer_backend>::get()->get_animation(asset_handler->get_asset_hash(anim.animation_asset_id));
        }
      } else if (!anim.clip_name.empty()) {
        resolved = source->find_clip(anim.clip_name);
      }

      const skeleton& skel = source->source_data().skel;
      if (resolved == nullptr) {
        anim.clip = nullptr;
        anim.bound_skeleton = nullptr;
        return;
      }

      if (anim.clip != resolved || anim.bound_skeleton != &skel) {
        /// first sight of this clip, or a hot reload swapped the clip/model underneath
        anim.clip = resolved;
        anim.bound_skeleton = &skel;
        anim.binding.build(*resolved, skel);
      }

      if (anim.playing) {
        anim.time += static_cast<float>(delta_time) * anim.speed;
        const float duration = anim.clip->duration;
        if (duration <= 0.f) {
          anim.time = 0.f;
        } else if (anim.looping) {
          anim.time = std::fmod(anim.time, duration);
          if (anim.time < 0.f) {  // negative speed wraps in from the end
            anim.time += duration;
          }
        } else {
          anim.time = std::clamp(anim.time, 0.f, duration);
        }
      }

      anim.working_pose.reset_to_bind(skel);
      sample_clip(*anim.clip, anim.binding, anim.time, anim.working_pose);

      render.obj_model.bone_matrices.resize(skel.joints.size());
      build_palette(skel, anim.working_pose, std::span<glm::mat4>{ render.obj_model.bone_matrices.data(), render.obj_model.bone_matrices.size() });
    }

  }  // namespace

  void scene::update(double delta_time, scope<asset_handler>& asset_handler) {
    ASSERT_MAIN_THREAD();
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::update");

    // check_synchronization_updates();

    if (playing && storage->physics != nullptr) {
      revalidate_physics();  /// live settings/shape edits apply at frame granularity

      /// gaffer-on-games accumulator
      /// ported from the deleted physx backend's single-step accumulator, upgraded to a catch-up loop
      constexpr static uint32_t kMaxCatchUpSteps = 5;
      const double step = subsystem<physics_environment>::get()->get_fixed_step();

      fixed_accumulator += delta_time;
      uint32_t steps = 0;
      while (fixed_accumulator >= step && steps < kMaxCatchUpSteps) {
        fixed_update(step);  /// full fixed tick: physics + scripts (§4)
        fixed_accumulator -= step;
        ++steps;
      }

      if (steps == kMaxCatchUpSteps && fixed_accumulator >= step) {
        /// death-spiral guard: drop the debt, keep alpha sane
        CORE_LOG_WARN("scene '{}' dropped {:.1f}ms of simulation time", name, 1000.0 * (fixed_accumulator - step));
        fixed_accumulator = std::fmod(fixed_accumulator, step);
      }
      /// presentation sync: entity transforms show the buffered fixed-step poses blended up
      ///  to this frame's leftover time; physics state itself only advances in fixed_update
      sync_physics_transforms(static_cast<float>(fixed_accumulator / step));
    }

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.update(delta_time);
    });

    if ((*storage->sandbox)["OnSceneUpdate"].valid()) {
      sol::protected_function on_update_fn = (*storage->sandbox)["OnSceneUpdate"];
      sol::protected_function_result result = on_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    /// after the script view and the lua scene hook: every script surface that drives
    ///  animation state ran this frame before sampling
    storage->registry.view<animation_component, render_component>().each([delta_time, &asset_handler](entt::entity entity, animation_component& anim, render_component& render) {
      tick_animation(anim, render, delta_time, asset_handler);
    });
  }

  void scene::late_update(double delta_time) {
    ASSERT_MAIN_THREAD();
    if (!playing) {
      return;
    }
    PROFILE_SECTION("scene::late_update");

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.late_update(delta_time);
    });

    if ((*storage->sandbox)["OnSceneLateUpdate"].valid()) {
      sol::protected_function on_late_update_fn = (*storage->sandbox)["OnSceneLateUpdate"];
      sol::protected_function_result result = on_late_update_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnLateUpdate' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  void scene::render_update(double delta_time) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::render_update");

    storage->registry.view<script_component>().each([delta_time](entt::entity entity, script_component& comp) {
      comp.render_update(delta_time);
    });

    if (sol::protected_function on_render_fn = (*storage->sandbox)["OnSceneRender"]; on_render_fn.valid()) {
      sol::protected_function_result result = on_render_fn(delta_time);
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneRender' for scene [{}:{}]", id, name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }
  }

  scene_object& scene::root_object() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_root_object");

    scene_tree::node* root_node = storage->tree.node_at(0);
    OTHER_ASSERT(root_node != nullptr, "Root node does not exist in the scene storage->tree.");
    OTHER_ASSERT(root_node->object != nullptr, "Root node object is null.");

    return *root_node->object;
  }

  scene_object& scene::create_object(const std::string& name, scene_object* parent_object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::create_object_with_name");
    return create_object(name, glm::vec3(0.f), parent_object);
  }

  scene_object& scene::create_object(const std::string& name, const glm::vec3& world_position, scene_object* parent_object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::create_object");
    return storage->tree.create_object(name, world_position, parent_object);
  }

  scene_object& scene::add_object(scene_object* object, const transform& transformation, scene_object* parent_object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::add_object");
    return storage->tree.add_object(object, transformation, parent_object);
  }

  scene_object* scene::get_parent(natural_t id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_parent");
    return storage->tree.get_parent(id);
  }

  const scene_object* scene::get_parent(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_parent_const");
    return const_cast<scene*>(this)->get_parent(id);
  }

  scene_object* scene::get_parent(scene_object* object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_parent");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  const scene_object* scene::get_parent(const scene_object* object) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_parent_const");
    if (object == nullptr) {
      return nullptr;
    }
    return get_parent(object->id);
  }

  ostd::vector<uint64_t> scene::get_children_ids(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_children_ids");
    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    ostd::vector<uint64_t> children_ids;
    for (const scene_tree::node* child : node->children) {
      if (child != nullptr && child->object != nullptr) {
        children_ids.push_back(child->object->id);
      }
    }
    return children_ids;
  }

  ostd::vector<uint64_t> scene::get_children_ids(const scene_object* object) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_children_ids");
    if (object == nullptr) {
      return {};
    }
    return get_children_ids(object->id);
  }

  ostd::vector<scene_object*> scene::get_children(natural_t id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_children");

    const scene_tree::node* node = storage->tree.node_at(id);
    if (node == nullptr) {
      CORE_LOG_ERROR("Node with the given ID does not exist in the scene storage->tree.");
      return {};
    }

    ostd::vector<scene_object*> children;
    for (const scene_tree::node* child : node->children) {
      if (child != nullptr && child->object != nullptr) {
        children.push_back(child->object);
      }
    }
    return children;
  }

  scene_object* scene::find_object_with_tag(const std::string_view tag) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::find_object_with_tag");
    for (const auto& id : get_all_object_ids()) {
      scene_object* obj = find_object(id);
      if (obj != nullptr && object_has_tag(id, tag)) {
        return obj;
      }
    }
    return nullptr;
  }

  ostd::vector<scene_object*> scene::get_children(const scene_object* object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_children");

    if (object == nullptr) {
      return {};
    }
    return get_children(object->id);
  }

  ostd::vector<uint64_t> scene::get_all_object_ids() const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_all_object_ids");
    return storage->tree.get_all_object_ids();
  }

  bool scene::is_visible(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::is_visible");
    const scene_tree::node* node = storage->tree.node_at(id);
    if (node == nullptr || node->object == nullptr) {
      return false;
    }
    return node->object->visible;
  }

  void scene::destroy_object(natural_t id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::destroy_object");
    storage->tree.destroy_object(id);
  }

  bool scene::reparent_object(natural_t id, natural_t new_parent_id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::reparent_object");

    const glm::mat4 child_world = get_world_transform(id);
    const glm::mat4 parent_world = get_world_transform(new_parent_id);

    if (!storage->tree.reparent(id, new_parent_id)) {
      return false;
    }

    /// recompute the local transform so the object does not visually move
    const glm::mat4 new_local = glm::inverse(parent_world) * child_world;

    transform& t = get_transform(id);
    t.local_position = glm::vec3(new_local[3]);
    glm::vec3 col0 = glm::vec3(new_local[0]);
    glm::vec3 col1 = glm::vec3(new_local[1]);
    glm::vec3 col2 = glm::vec3(new_local[2]);
    t.local_scale = { glm::length(col0), glm::length(col1), glm::length(col2) };
    constexpr float kMinScale = 1e-6f;
    const glm::mat3 rot{
      col0 / std::max(t.local_scale.x, kMinScale),
      col1 / std::max(t.local_scale.y, kMinScale),
      col2 / std::max(t.local_scale.z, kMinScale)
    };
    t.local_rotation_quat = glm::quat_cast(rot);
    return true;
  }

  bool scene::has_object(const std::string_view name) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::has_object_by_id");
    auto* node = storage->tree.find_object_by_name(name);
    return node != nullptr;
  }

  bool scene::has_object(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::has_object_by_id");
    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Scene nodes should never be null. Node with ID {} is null '{}'.", id, this->name);
    return node->object != nullptr;
  }

  scene_object& scene::get_object(const std::string_view name) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_object_by_name");
    auto* node = storage->tree.find_object_by_name(name);
    OTHER_ASSERT(node != nullptr, "Scene object with name '{}' not found in scene '{}'.", name, this->name);
    return *node;
  }

  const scene_object& scene::get_object(const std::string_view name) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_object_by_name_const");
    auto* node = storage->tree.find_object_by_name(name);
    OTHER_ASSERT(node != nullptr, "Scene object with name '{}' not found in scene '{}'.", name, this->name);
    return *node;
  }

  scene_object& scene::get_object(natural_t id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_object");
    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    /// a reset node means the id outlived its object (snapshot restores reassign runtime
    ///  ids) — callers that can hold stale ids must resolve through find_object instead
    OTHER_ASSERT(node->object != nullptr, "Scene object with ID {} no longer exists.", id);
    return *node->object;
  }

  const scene_object& scene::get_object(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_object_const");
    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    OTHER_ASSERT(node->object != nullptr, "Scene object with ID {} no longer exists.", id);
    return *node->object;
  }

  scene_object* scene::find_object(const std::string_view name) const {
    ASSERT_MAIN_THREAD();
    scene_object* n = storage->tree.find_object_by_name(name);
    return n != nullptr ? n : nullptr;
  }

  scene_object* scene::find_object(natural_t id) const {
    ASSERT_MAIN_THREAD();
    scene_tree::node* node = storage->tree.node_at(id);
    return (node != nullptr && node->object != nullptr) ? node->object : nullptr;
  }

  size_t scene::get_object_count() const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_object_count");
    OTHER_ASSERT(storage->tree.nodes != nullptr, "Scene tree nodes are not initialized.");
    return storage->tree.get_object_count();
  }

  camera* scene::get_primary_camera() {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_primary_camera");
    scene_object* obj = find_object_with_tag("main-camera");
    if (obj != nullptr) {
      camera_component* cam_comp = get_component<camera_component>(obj);
      if (cam_comp != nullptr) {
        return &cam_comp->camera;
      }
    }
    return nullptr;
  }

  const camera* scene::get_primary_camera() const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_primary_camera_const");
    const scene_object* obj = find_object_with_tag("main-camera");
    if (obj != nullptr) {
      const camera_component* cam_comp = get_component<camera_component>(obj);
      if (cam_comp != nullptr) {
        return &cam_comp->camera;
      }
    }
    return nullptr;
  }

  transform& scene::get_transform(scene_object* object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_transform");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    transform* t = storage->registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  const transform& scene::get_transform(const scene_object* object) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_transform_const");
    OTHER_ASSERT(object != nullptr, "Cannot get transform from a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    const transform* t = storage->registry.try_get<transform>(entity);
    OTHER_ASSERT(t != nullptr, "Transform component does not exist for the given scene object.");

    return *t;
  }

  void scene::set_transform(scene_object* object, const transform& t) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::set_transform");
    OTHER_ASSERT(object != nullptr, "Cannot set transform on a null scene object.");

    entt::entity entity = entt::entity(object->registry_id);
    storage->registry.replace<transform>(entity, t);
  }

  glm::mat4 scene::get_world_transform(scene_object* obj) const {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(obj != nullptr, "Cannot get world transform from a null scene object.");
    PROFILE_SECTION("scene::get_world_transform");

    return get_world_transform(obj->id);
  }

  glm::mat4 scene::get_world_transform(natural_t id) const {
    ASSERT_MAIN_THREAD();
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

  glm::mat4 scene::get_local_transform(scene_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot get local transform from a null scene object.");
    return get_local_transform(obj->id);
  }

  glm::mat4 scene::get_local_transform(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_local_transform");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");

    return get_transform(node->object).get_local_model_matrix();
  }

  glm::mat4 scene::get_local_to_world_matrix(scene_object* obj) const {
    OTHER_ASSERT(obj != nullptr, "Cannot get local to world matrix from a null scene object.");
    return get_local_to_world_matrix(obj->id);
  }

  glm::mat4 scene::get_local_to_world_matrix(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_local_to_world_matrix");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");

    if (scene_tree::node* parent_node = node->parent; parent_node != nullptr) {
      return get_world_transform(parent_node->id);
    } else {
      return glm::mat4(1.0f);
    }
  }

  transform& scene::get_transform(natural_t id) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_transform_by_id");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return get_transform(node->object);
  }

  const transform& scene::get_transform(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_transform_by_id");

    const scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return get_transform(node->object);
  }

  void scene::set_transform(natural_t id, const transform& t) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::set_transform_by_id");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    set_transform(node->object, t);
  }

  bounding_box scene::get_bounding_box(scene_object* object) const {
    OTHER_ASSERT(object != nullptr, "Scene object is null.");
    const entt::entity entity = entt::entity(object->registry_id);

    bounding_box box = bounding_box::empty;
    {
      const render_component* rc = storage->registry.try_get<render_component>(entity);
      if (rc != nullptr && rc->obj_model.source != nullptr) {
        const model_data& source_data = rc->obj_model.source->source_data();
        box = source_data.bounds;

        /// a live palette means the draw skins with it, so bound the animated pose: union
        ///  of each joint's influenced bounds through its palette matrix
        const ostd::vector<glm::mat4>& palette = rc->obj_model.bone_matrices;
        if (!palette.empty() && !source_data.skel.empty()) {
          bounding_box animated = bounding_box::empty;
          const size_t joint_count = std::min(palette.size(), source_data.skel.joints.size());
          for (size_t i = 0; i < joint_count; ++i) {
            bounding_box joint_bounds = source_data.skel.joints[i].influenced_bounds;
            if (joint_bounds == bounding_box::empty) {
              continue;
            }
            animated = bounding_box::expand_to_include(animated, joint_bounds.transform(palette[i]));
          }
          if (!(animated == bounding_box::empty)) {
            box = animated;
          }
        }
      }

      const physics_component* pc = storage->registry.try_get<physics_component>(entity);
      if (pc != nullptr && pc->shape != nullptr) {
        /// a collider drives the bounds only once it actually built (kNone/deferred stay empty)
        if (bounding_box shape_box = pc->shape->get_bounding_box(); !(shape_box == bounding_box::empty)) {
          box = shape_box;
        }
      }
    }

    if (box == bounding_box::empty) {
      return bounding_box(glm::vec3(-1.f), glm::vec3(1.f)).transform(get_world_transform(object));
    } else {
      return box.transform(get_world_transform(object));
    }
  }

  bounding_box scene::get_bounding_box(natural_t id) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_bounding_box_by_id");

    scene_tree::node* node = storage->tree.node_at(id);
    OTHER_ASSERT(node != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    return get_bounding_box(node->object);
  }

  bounding_box scene::get_bounding_box(std::span<scene_object*> objects) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_bounding_box_by_objects");

    bounding_box box;
    for (scene_object* obj : objects) {
      if (obj != nullptr) {
        box = bounding_box::expand_to_include(box, get_bounding_box(obj));
      }
    }
    return box;
  }

  bounding_box scene::get_bounding_box(std::span<const natural_t> ids) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_bounding_box_by_ids");

    bounding_box box;
    for (natural_t id : ids) {
      scene_tree::node* node = storage->tree.node_at(id);
      if (node != nullptr) {
        box = bounding_box::expand_to_include(box, get_bounding_box(node->object));
      }
    }
    return box;
  }

  bounding_box scene::get_bounding_box() const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_bounding_box_all");

    bounding_box box;
    for (const scene_tree::node& n : *storage->tree.nodes) {
      if (n.object != nullptr) {
        box = bounding_box::expand_to_include(box, get_bounding_box(n.object));
      }
    }
    return box;
  }

  bounding_box scene::get_bounding_box_from_camera_frustum(const camera& cam) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::get_bounding_box_from_camera_frustum");
    // this will contain the entire frustum even which gives weird results if far plane is very far away
    // we cut it off to give a 'local' camera frustum
    bounding_box bbox = cam.get_frustum().get_containing_aabb();

    constexpr float max_distance = 10.f;
    if (bbox.min.z < 0.f) {
      bbox.min.z = 0.f;
    }
    if (bbox.max.z < bbox.min.z) {
      bbox.max.z = 10.f;
    }
    if (bbox.max.z > max_distance) {
      bbox.max.z = max_distance;
    }

    /// make x and y sorta proportional w/ z so we get a more 'natural' box that isn't super long and flat
    const float z_range = bbox.max.z - bbox.min.z;
    const float y_center = (bbox.max.y + bbox.min.y) / 2.f;
    bbox.min.y = y_center - z_range / 2.f;
    bbox.max.y = y_center + z_range / 2.f;

    const float x_center = (bbox.max.x + bbox.min.x) / 2.f;
    bbox.min.x = x_center - z_range / 2.f;
    bbox.max.x = x_center + z_range / 2.f;

    return bbox;
  }

  render_data scene::prepare_render_data(scope<asset_handler>& asset_handler) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::prepare_render_data");

    render_data data;
    data.clear_color = storage->clear_color;

    const camera* primary_camera = get_primary_camera();

    storage->registry.view<object_handle, point_light_component>().each([&](const object_handle& handle, const point_light_component& light) {
      glm::vec3 world_pos = get_world_transform(handle.id) * glm::vec4(light.light.position, 1.f);
      gpu::light l{
        .vector = { world_pos.x, world_pos.y, world_pos.z, 0.f },
        .color = light.light.color,
        .light_type = gpu::light::kPoint,
      };
      data.lights.push_back(l);
    });

    const direction_light* scene_ambient_light = nullptr;
    storage->registry.view<object_handle, direction_light_component>().each([&](const object_handle& handle, const direction_light_component& light) {
      /// every directional light shades through the light buffer; vector.xyz keeps the
      ///  component's TO-the-light convention (the same one sim env's sun_direction uses)
      gpu::light l{
        .vector = glm::vec4(light.light.direction, 0.f),
        .color = light.light.color,
        .light_type = gpu::light::kDirection,
      };
      data.lights.push_back(l);

      const bool sun_tag = object_has_tag(handle.id, "sun");
      if (scene_ambient_light == nullptr && sun_tag) {
        scene_ambient_light = &light.light;
      } else if (sun_tag) {
        CORE_LOG_WARN("Multiple directional lights with the 'sun' tag detected. Using the first one as the scene ambient light.");
      }
    });

    bool instance_overflow_warned = false;
    storage->registry.view<object_handle, render_component>().each([&](const object_handle& handle, render_component& render) {
      if (!render.visible) {
        return;
      }
      auto& obj = get_object(handle.id);
      if (!obj.visible) {
        return;
      }

      bool changed = render.last_model_asset_id != render.model_asset_id;
      if (changed) {
        if (!asset_handler->asset_exists(render.model_asset_id)) {
          CORE_LOG_ERROR("Render component model asset ID {} does not exist for object ID {}.", render.model_asset_id, handle.id);
          // avoids repeated failed lookups and objects don't disappear from scene
          render.model_asset_id = render.last_model_asset_id;
        }
      }

      /// the same validation dance for the material override; a bad assignment reverts
      ///  instead of killing the draw
      if (render.last_material_asset_id != render.material_asset_id && render.material_asset_id != 0) {
        if (!asset_handler->asset_exists(render.material_asset_id)) {
          CORE_LOG_ERROR("Render component material asset ID {} does not exist for object ID {}.", render.material_asset_id, handle.id);
          render.material_asset_id = render.last_material_asset_id;
        }
      }
      render.last_material_asset_id = render.material_asset_id;

      /// either they are the same or we already validated the change
      const bool is_loaded = asset_handler->asset_loaded(render.model_asset_id);
      if (!is_loaded) {
        return;
      }

      natural_t hash = asset_handler->get_asset_hash(render.model_asset_id);
      OTHER_ASSERT(hash != 0, "Asset hash is 0 for asset ID {}.", render.model_asset_id);

      renderer_backend* backend = subsystem<renderer_backend>::get();
      ref<model_source> model_src = backend->get_model_source(hash);
      if (model_src == nullptr) {
        /// unloaded from the renderer (or mid-reload): drop the draw and forget the stale instance
        render.obj_model = {};
        return;
      }
      if (render.obj_model.source != model_src.raw_ptr()) {
        /// first sight of this asset, or a hot reload swapped the source under the same hash
        render.obj_model = model_src->produce_model();
      }

      /// effective material resolution: component override wins when its asset is registered,
      ///  else the model's imported material per submesh, else nullptr = layout defaults at
      ///  bind time. the key keeps batching stable through the override's async load window.
      const material* override_material = nullptr;
      natural_t material_key = 0;
      if (render.material_asset_id != 0 && asset_handler->asset_exists(render.material_asset_id)) {
        material_key = asset_handler->get_asset_hash(render.material_asset_id);
        override_material = backend->get_material(material_key);
      }

      model* draw_model = &render.obj_model;
      const std::span<const submesh> submeshes = draw_model->source->source_data().submeshes;
      OTHER_ASSERT(!submeshes.empty(), "Model source has no submeshes");
      const ostd::vector<material>& imported_materials = model_src->imported_materials();

      const auto sm_idxs = draw_model->submesh_indices;
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
          .material_key = material_key,
        };

        const submesh& sm = submeshes[sm_idx];
        const material* draw_material = override_material;
        if (draw_material == nullptr && sm.material_index < imported_materials.size()) {
          draw_material = &imported_materials[sm.material_index];
        }

        auto it = data.mesh_indices.find(key);
        if (it == data.mesh_indices.end()) {
          auto [itr, inserted] = data.mesh_indices.insert({ key, data.num_draw_calls++ });
          OTHER_ASSERT(inserted, "Failed to insert mesh key into map");

          data.mesh_keys.emplace_back() = key;
          data.draw_calls.emplace_back() = draw_call{};
          data.draw_materials.emplace_back() = draw_material;
          data.draw_tints.emplace_back() = draw_instance_tints{};
          data.model_buffers.emplace_back() = gpu::model_matrix_buffer{};
          data.bone_buffers.emplace_back() = gpu::bone_matrix_buffer{};

          it = itr;
        }
        OTHER_ASSERT(it != data.mesh_indices.end(), "Mesh key not found in map after insertion");

        size_t mesh_index = it->second;

        draw_call& call = data.draw_calls[mesh_index];
        if (call.instance_count == 0) {
          call.submesh_index = sm_idx;

          call.mesh_handle = draw_model->source->get_mesh_handle();

          call.vertex_offset = sm.base_vertex;
          call.vertex_count = sm.vert_cnt;
          call.index_offset = sm.base_idx;
          call.index_count = sm.idx_cnt;

          call.line_thickness = 1.f;
        }

        /// per-instance slots (tints + model matrices) are fixed arrays: a dropped instance
        ///  beats a buffer overrun; a real >kMaxMaterials-instance path is instancing work
        if (call.instance_count >= gpu::kMaxMaterials) {
          if (!instance_overflow_warned) {
            CORE_LOG_WARN("Draw for submesh {} exceeded {} instances; extra instances are dropped this frame.", sm_idx, gpu::kMaxMaterials);
            instance_overflow_warned = true;
          }
          continue;
        }

        glm::mat4 world_transform = get_world_transform(handle.id);  // * transform_it->second;

        size_t index = call.instance_count++;
        data.draw_tints[mesh_index].tints[index] = render.tint;
        data.model_buffers[mesh_index].model_matrices[index] = world_transform;

        /// instances sharing one draw share one palette (per-draw buffer); the tick
        ///  recomputes it every frame, so no clear. unrigged draws keep use_bones = 0
        if (sm.rigged && !draw_model->bone_matrices.empty()) {
          gpu::bone_matrix_buffer& bone_buff = data.bone_buffers[mesh_index];
          const size_t bone_count = std::min(draw_model->bone_matrices.size(), kMaxBones);
          std::copy_n(draw_model->bone_matrices.begin(), bone_count, bone_buff.bone_matrices);
          bone_buff.use_bones = 1;
        }
      }

      render.last_model_asset_id = render.model_asset_id;
    });

    if (scene_ambient_light != nullptr) {
      data.simulation_environment.sun_direction = glm::vec4(scene_ambient_light->direction, 0.0f);
      data.simulation_environment.sun_color = scene_ambient_light->color;
    }

    /// average of all light contributions for now, we can do something more complex later if needed
    data.simulation_environment.ambient_color = glm::vec4(0.f);
    for (const auto& light : data.lights) {
      data.simulation_environment.ambient_color += light.color;
    }
    data.simulation_environment.ambient_color += data.simulation_environment.sun_color;
    data.simulation_environment.ambient_color /= static_cast<float>(data.lights.size() + 1);

    constexpr float kEnvHalfExtent = 16.0f;
    const float half = kEnvHalfExtent;
    const glm::vec3 voxel = glm::vec3(2.0f * half / 64.0f);

    data.primary_camera = const_cast<camera*>(primary_camera);

    bounding_box scene_bounding_box = {};
    glm::vec3 c = {};
    if (data.primary_camera == nullptr) {
      scene_bounding_box = get_bounding_box();
      c = (scene_bounding_box.min + scene_bounding_box.max) / 2.0f;
    } else {
      scene_bounding_box = get_bounding_box_from_camera_frustum(*primary_camera);
      c = primary_camera->center();
    }

    c = glm::round(c / voxel) * voxel;
    data.simulation_environment.world_min = glm::vec4(c - half, 1.0f);
    data.simulation_environment.world_max = glm::vec4(c + half, 1.0f);  // exposure);

    return data;
  }

  bool scene::object_has_tag(natural_t id, const std::string_view tag) const {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::object_has_tag");
    return storage->tree.node_has_tag(id, tag);
  }

  void scene::add_object_tag(natural_t id, const std::string_view tag) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::add_object_tag");
    scene_tree::node* n = storage->tree.node_at(id);
    OTHER_ASSERT(n != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    n->tags.push_back(object_tag{ std::string{ tag } });
  }

  void scene::remove_object_tag(natural_t id, const std::string_view tag) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::remove_object_tag");
    scene_tree::node* n = storage->tree.node_at(id);
    OTHER_ASSERT(n != nullptr, "Node with the given ID does not exist in the scene storage->tree.");
    n->tags.erase(std::remove_if(n->tags.begin(), n->tags.end(), [&](const object_tag& t) { return t.name == tag; }), n->tags.end());
  }

  std::string scene::as_string(const scene& s) {
    ASSERT_MAIN_THREAD();
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
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::connect_remote_session");

    scene_object& root = root_object();

    auto* net_ctx = get_component<scene_network_context>(&root);
    OTHER_ASSERT(net_ctx != nullptr, "Scene network context component is not present on the root scene object.");

    net_ctx->add_remote_session(session_id);
  }

  scene::object_handle::operator scene_object*() const {
    ASSERT_MAIN_THREAD();
    OTHER_ASSERT(object != nullptr, "Object handle is null, cannot convert to scene_object*.");
    return object;
  }

  bool scene::object_handle::operator==(const object_handle& other) const {
    ASSERT_MAIN_THREAD();
    return id == other.id && object == other.object;
  }

  scene_object* scene::from_registry_id(entt::entity entity) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::from_registry_id");

    object_handle* handle = storage->registry.try_get<object_handle>(entity);
    if (handle == nullptr) {
      CORE_LOG_ERROR("Object handle not found for entity {}", (natural_t)entity);
      return nullptr;
    }
    return handle->object;
  }

  void scene::register_object(scene_object* object, const std::string& name, const glm::vec3& world_position) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::register_object");

    OTHER_ASSERT(name.size() > 0, "Scene object name cannot be empty.");
    OTHER_ASSERT(object != nullptr, "Cannot register a null scene object.");

    CORE_LOG_DEBUG(" - registering scene object '{}' in scene '{}'", name, this->name);
    entt::entity entity = storage->registry.create();
    object->name = name;
    object->registry_id = (uint32_t)entity;

    // object handle and component registery are 'invisible' components (user should not know about them)
    storage->registry.emplace<object_handle>(entity, object_handle{ .id = object->id, .object = object });
    storage->registry.emplace<object_component_registry>(entity, object_component_registry{});
    storage->registry.emplace<transform>(entity, transform{
                                                   orthonormal_basis(glm::vec3(0, 1, 0)),
                                                   world_position,
                                                   glm::vec3(1, 1, 1),
                                                   glm::quat(),
                                                 });
    storage->registry.emplace<script_component>(entity, script_component{ object });

    transform& transf = storage->registry.get<transform>(entity);
    script_component& script = storage->registry.get<script_component>(entity);

    auto& comp_reg = storage->registry.get<object_component_registry>(entity);
    comp_reg.register_component(transf);
    comp_reg.register_component(script);
  }

  void scene::register_object(scene_object* object, const std::string& name, const transform& transformation) {
    ASSERT_MAIN_THREAD();
    register_object(object, name, transformation.local_position);
    set_transform(object, transformation);
  }

  void scene::unregister_object(scene_object* object) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::unregister_object");
    {
      // auto& comb_reg = get_component<object_component_registry>(object);
      // comp_reg.unregister_all();
    }

    storage->registry.remove<script_component>(entt::entity(object->registry_id));
    storage->registry.remove<transform>(entt::entity(object->registry_id));
    storage->registry.remove<object_component_registry>(entt::entity(object->registry_id));
    storage->registry.remove<object_handle>(entt::entity(object->registry_id));
    storage->registry.destroy(entt::entity(object->registry_id));
  }

  void scene::on_create_render_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
  }

  void scene::on_update_render_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
  }

  void scene::on_destroy_render_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
  }

  void scene::on_create_script_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::on_create_script_component");

    script_component* script = storage->registry.try_get<script_component>(entity);
    if (script == nullptr) {
      CORE_LOG_ERROR("Script component not found for entity {}", (natural_t)entity);
      return;
    }
    OTHER_ASSERT(script->object != nullptr, "Scene object reference in script component is null for entity {}", (natural_t)entity);

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    if (preserving_script_objects) {
      for (auto it = preserved_script_objects.begin(); it != preserved_script_objects.end(); ++it) {
        script_object* preserved = script_env->get_object(*it);
        if (preserved != nullptr && preserved->name == script->object->name) {
          script->script_object_id = *it;
          script_env->rebind_dotnet_object(*it, (void*)script->object);
          preserved_script_objects.erase(it);
          CORE_LOG_DEBUG("Rebound script object with ID {} for scene object '{}' [ID: {}] (entity {})", script->script_object_id, script->object->name, script->object->id, (natural_t)entity);
          return;
        }
      }
    }

    script->script_object_id = script_env->create_object(script->object->name);
    CORE_LOG_DEBUG("Created script object with ID {} for scene object '{}' [ID: {}] (entity {})", script->script_object_id, script->object->name, script->object->id, (natural_t)entity);

    script_object* object = script_env->get_object(script->script_object_id);
    OTHER_ASSERT(object != nullptr, "Failed to retrieve script object after creation for object ID {}", script->script_object_id);

    script_env->attach_dotnet_object(script->script_object_id, "Other.SceneObject", (void*)script->object);
  }

  // void scene::on_update_script_component(const entt::registry&, const entt::entity entity) {
  // }

  void scene::on_destroy_script_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::on_destroy_script_component");

    auto* script_env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(script_env != nullptr, "Scripting environment is not initialized.");

    script_component& script = storage->registry.get<script_component>(entity);
    if (preserving_script_objects && script.script_object_id >= 0) {
      /// play-stop restore: the managed instance outlives the native rebuild and is
      ///  reclaimed by name in on_create_script_component
      preserved_script_objects.push_back(script.script_object_id);
      return;
    }
    script_env->destroy_object(script.script_object_id);
  }

  void scene::on_create_physics_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::on_create_physics_component");

    physics_component& physics_comp = storage->registry.get<physics_component>(entity);
    if (storage->physics == nullptr) {
      return;  /// physics-off profile: the component stays inert data
    }

    object_handle& obj_handle = storage->registry.get<object_handle>(entity);

    physics_comp.body = storage->physics->create_physics_body(physics_comp.settings, get_world_transform(obj_handle.id));
    OTHER_ASSERT(physics_comp.body != nullptr, "Failed to create physics body for entity {}", (natural_t)entity);
    physics_comp.body->owner_object_id = obj_handle.id;
    physics_comp.body->active = true;
    apply_component_shape(entity, physics_comp);
  }

  // void scene::on_update_physics_component(const entt::registry&, const entt::entity entity) {}

  void scene::on_destroy_physics_component(const entt::registry&, const entt::entity entity) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("scene::on_destroy_physics_component");

    physics_component& physics_comp = storage->registry.get<physics_component>(entity);
    if (storage->physics == nullptr || physics_comp.body == nullptr) {
      return;  /// physics-off profile, or a component that never got a body
    }

    if (physics_comp.shape != nullptr) {
      storage->physics->destroy_physics_shape(physics_comp.shape);
    }
    storage->physics->destroy_physics_body(physics_comp.body);
    physics_comp.shape = nullptr;
    physics_comp.body = nullptr;
  }

}  // namespace other