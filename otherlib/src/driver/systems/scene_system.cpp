/**
 * \file driver/systems/scene_system.cpp
 **/
#include "driver/systems/scene_system.hpp"

#include <sol/types.hpp>

#include "serialization/scene_serializer.hpp"

#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/grid_component.hpp"
#include "object/physics_joint_component.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/project_system.hpp"
#include "scripting/scene_interface.hpp"

namespace other {

  void scene_system::initialize(driver_kernel* kernel) {
    PROFILE_SECTION("scene_system::initialize");
    project_scene_graph = make_scope<scene_graph>();
    OTHER_ASSERT(project_scene_graph != nullptr, "Failed to create project scene graph.");

    component_reg = make_scope<component_registry>();
    OTHER_ASSERT(component_reg != nullptr, "Failed to create component registry for scene system.");

    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    events->register_event("scene.load-scene");
    events->add_listener("scene.load-scene", std::bind_front(&scene_system::handle_scene_load_event, this));
    events->register_event("scene.asset-loaded");
    events->add_listener("scene.asset-loaded", std::bind_front(&scene_system::handle_scene_asset_loaded_event, this));
    events->register_event("scene.asset-unloaded");
    events->add_listener("scene.asset-unloaded", std::bind_front(&scene_system::handle_scene_asset_unloaded_event, this));

    events->register_event("scene.unload-scene");
    events->add_listener("scene.unload-scene", std::bind_front(&scene_system::handle_scene_unload_event, this));
    events->register_event("scene.request-info");
    events->add_listener("scene.request-info", std::bind_front(&scene_system::handle_scene_info_event, this));
    events->register_event("scene.playback-command");
    events->add_listener("scene.playback-command", std::bind_front(&scene_system::handle_scene_playback_command_event, this));

    events->register_event("scene.activated");
    events->register_event("scene.deactivated");

    events->register_event("ls.scenes");
    events->add_listener("ls.scenes", [this](const value& data) { handle_ls_scenes_event(&get_driver().get_kernel(), data); });

    register_components();

    scene_interface::initialize(&get_driver());
  }

  void scene_system::tick(driver_kernel* kernel, double dt) {
    PROFILE_SECTION("scene_system::tick");
    if (scene* active_scene = get_active_scene(); active_scene != nullptr) {
      OTHER_ASSERT(kernel->has_core_system<asset_system>(), "Asset system is not available in driver kernel.");
      active_scene->update(dt, sibling<asset_system>(*kernel).get_asset_manager());
      active_scene->late_update(dt);
      /// not gated on playback, scripts draw debug/scene overlays every frame
      active_scene->render_update(dt);
    }
  }

  void scene_system::shutdown(driver_kernel* kernel) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    OTHER_ASSERT(active_scene == nullptr, "There is an active scene. Cannot shutdown scene system while a scene is active.");
    PROFILE_SECTION("scene_system::shutdown");
    project_scene_graph->clear();
    project_scene_graph = nullptr;
  }

  void scene_system::load_project_scene_graph(project& p) {
    PROFILE_SECTION("scene_system::load_project_scene_graph");
    for (auto& scene_data : p.get_scenes()) {
      if (!std::filesystem::exists(scene_data.path)) {
        CORE_LOG_ERROR("Scene file '{}' for scene '{}' in project does not exist. Skipping loading this scene.", scene_data.path.string(), scene_data.name);
        continue;
      }

      natural_t id = add_scene_to_scene_graph(scene_data.path);
      OTHER_ASSERT(scene_data.scene_id == id, "Scene ID mismatch for scene '{}'. Expected {}, got {}.", scene_data.name, scene_data.scene_id, id);
      CORE_LOG_DEBUG("Loaded scene '{}' with ID {} from project.", scene_data.name, id);
    }
  }

  natural_t scene_system::add_scene_to_scene_graph(const filepath& scene_path) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    PROFILE_SECTION("scene_system::add_scene_to_scene_graph");
    if (!std::filesystem::exists(scene_path)) {
      CORE_LOG_ERROR("Scene file does not exist: {}", scene_path.string());
      CORE_LOG_ERROR("Can not add scene '{}' to scene graph", scene_path.filename().stem().string());
      return 0;
    }

    if (!serialization::is_scene_file_extension(scene_path.extension().string())) {
      CORE_LOG_ERROR("Scene file '{}' is not a scene document ({}/{} expected).", scene_path.string(), serialization::kSceneTomlExtension, serialization::kSceneBinaryExtension);
      CORE_LOG_ERROR("Lua scene files are no longer loadable — migrate the scene to a .oscn document and reference the lua as its behavior-hook `script`.");
      return 0;
    }

    if (project_scene_graph->has_scene(scene_path.filename().stem().string())) {
      CORE_LOG_DEBUG("Scene '{}' already exists in scene graph.", scene_path.filename().stem().string());
      return FNV(scene_path.filename().stem().string());
    }

    auto id = create_empty_scene(scene_path.filename().stem().string(), false);
    OTHER_ASSERT(id != 0, "Failed to add scene [{}] to scene graph.", scene_path.string());
    CORE_LOG_DEBUG("Added scene [{}] to scene graph with ID {}", scene_path.string(), id);

    auto* s = get_scene(id);
    OTHER_ASSERT(s != nullptr, "Failed to retrieve scene [{}] after adding to scene graph.", scene_path.string());

    get_driver().add_scene_asset(s, scene_path);
    return id;
  }

  natural_t scene_system::create_empty_scene(const std::string_view name, bool add_asset) {
    PROFILE_SECTION("scene_system::create_empty_scene");
    auto [id, ptr] = project_scene_graph->create_new_scene(name);
    if (add_asset) {
      get_driver().add_scene_asset(ptr);
    }
    return id;
  }

  natural_t scene_system::get_id_of_scene(const std::string_view name) {
    return project_scene_graph->get_id_of_scene(name);
  }

  scene* scene_system::get_scene(natural_t id) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    return project_scene_graph->find_scene(id);
  }

  void scene_system::set_scene_to_active(natural_t scene_id) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    PROFILE_SECTION("scene_system::set_scene_to_active");
    if (active_scene != nullptr && active_scene->id == scene_id) {
      CORE_LOG_DEBUG("Scene [{}:{}] is already active, no need to set active again.", active_scene->id, active_scene->name);
      return;
    }

    if (active_scene != nullptr) {
      CORE_LOG_DEBUG("Another scene [{}:{}] is already active, unloading it first.", active_scene->id, active_scene->name);
      unload_active_scene();
    }
    CORE_LOG_DEBUG("Setting scene [{}] to active.", scene_id);

    active_scene = project_scene_graph->find_scene(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Scene with ID {} not found in scene graph.", scene_id);
    CORE_LOG_DEBUG("Scene [{}:{}] Activation.", active_scene->id, active_scene->name);

    lua_sandbox& sandbox = active_scene->get_sandbox();
    opt<sol::table> native_table = sandbox["__other_native"];
    if (native_table.has_value() && native_table->valid()) {
      sol::table scene_table = sandbox["__other_native"]["__native_scene"];
      sol::table scene_interface_table = sandbox["__other_native"]["__scene_interface"];

      scene_table["__native_pointer"] = active_scene;
      scene_table.set_function(
        "create_scene_object",
        sol::overload(
          [s = active_scene](const std::string& name) -> natural_t {
            OTHER_ASSERT(s != nullptr, "Active scene is null.");
            CORE_LOG_DEBUG("Lua scene boundary: creating scene object with name '{}'", name);
            return s->create_object(name).id;
          },
          [s = active_scene](const std::string& name, const glm::vec3& world_position) -> natural_t {
            OTHER_ASSERT(s != nullptr, "Active scene is null.");
            CORE_LOG_DEBUG("Lua scene boundary: creating scene object with name '{}' at position ({}, {}, {})", name, world_position.x, world_position.y, world_position.z);
            return s->create_object(name, world_position).id;
          },
          [s = active_scene](const std::string& name, const glm::vec3& world_position, natural_t parent_id) -> natural_t {
            OTHER_ASSERT(s != nullptr, "Active scene is null.");
            CORE_LOG_DEBUG("Lua scene boundary: creating parented scene object with name '{}' at position ({}, {}, {}) with parent ID {}", name, world_position.x, world_position.y, world_position.z, parent_id);
            return s->create_object(name, world_position, &s->get_object(parent_id)).id;
          }));
    } else {
      CORE_LOG_WARN("Scene native binding table '__other_native' is invalid.");
    }

    /// declarative content first, then the behavior-hook script — both only happen the
    ///  first time the scene activates
    active_scene->instantiate_pending_document();
    active_scene->run_script_file();

    auto& storage = active_scene->get_storage();
    if (storage.sandbox["OnSceneActivate"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneActivate' for scene [{}:{}]", active_scene->id, active_scene->name);
      sol::protected_function on_scene_activate_fn = storage.sandbox["OnSceneActivate"];
      sol::protected_function_result result = on_scene_activate_fn();
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneActivate' for scene [{}:{}]", active_scene->id, active_scene->name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    auto& events = get_driver().get_event_system();
    events->trigger_event("scene.activated", active_scene->id);
    get_driver().on_scene_activated(active_scene->id);

    if (get_driver().should_auto_play_scenes()) {
      CORE_LOG_DEBUG("Auto-playing scene [{}:{}] on activation.", active_scene->id, active_scene->name);
      active_scene->play();
      get_driver().on_scene_played(active_scene->id);
    }
  }

  void scene_system::synchronize_active_scene(natural_t scene_id) {
    PROFILE_SECTION("scene_system::synchronize_active_scene");
    bool network_thread_active = get_driver().network_enabled();
    /// \todo should we assert instead of return?
    /// OTHER_ASSERT(get_driver().network_enabled(), "Should not be attempting to synchronize scene because network thread is not active.");
    if (!network_thread_active) {
      return;
    }

    // /// if we are a client and are connected to the server send the load command, if we are client and
    // ///  are not connected to a server we still set synchronized to false in case of a connection later
    // ///  we know to begin synchronization
    // if (network_thread_active && primary_role == driver_role::CLIENT) {
    //   if (client_session_id.has_value()) {
    //     constexpr bool is_empty = false;
    //     constexpr bool requires_udp_binding = true;
    //     send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
    //   }

    //   active_scene->synchronized = false;
    // }
    // /// if we are a server and have clients connected send the load command to them
    // else if (network_thread_active &&
    //          primary_role == driver_role::SERVER && !app_list.other_apps.empty()) {
    //   for (const auto& [other_app_id, other_app] : app_list.other_apps) {
    //     if (!other_app.connected) {
    //       continue;
    //     }
    //     constexpr bool is_empty = false;
    //     constexpr bool requires_udp_binding = true;
    //     send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
    //   }
    // }
  }

  void scene_system::unload_active_scene() {
    PROFILE_SECTION("scene_system::unload_active_scene");
    if (active_scene == nullptr) {
      return;
    }

    if (active_scene->is_playing()) {
      active_scene->stop();
      active_scene->reset();
    }

    auto& storage = active_scene->get_storage();
    if (storage.sandbox["OnSceneDeactivate"].valid()) {
      CORE_LOG_DEBUG("Calling 'OnSceneDeactivate' for scene [{}:{}]", active_scene->id, active_scene->name);
      sol::protected_function on_scene_deactivate_fn = storage.sandbox["OnSceneDeactivate"];
      sol::protected_function_result result = on_scene_deactivate_fn();
      if (!result.valid()) {
        CORE_LOG_ERROR("Failed to execute 'OnSceneDeactivate' for scene [{}:{}]", active_scene->id, active_scene->name);
        sol::error err = result;
        CORE_LOG_ERROR("Lua Error: {}", err.what());
      }
    }

    lua_sandbox& sandbox = active_scene->get_sandbox();
    opt<sol::table> native_table = sandbox["__other_native"];
    if (native_table.has_value() && native_table->valid()) {
      sol::table scene_table = sandbox["__other_native"]["__native_scene"];
      sol::table scene_interface_table = sandbox["__other_native"]["__scene_interface"];

      scene_table["__native_pointer"] = sol::nil;
      scene_table.set_function(
        "create_scene_object",
        sol::overload(
          [](const std::string& name) -> natural_t {
            OTHER_ASSERT(false, "Active scene is null. Should not be accessing scene through lua script with no scene active.");
          },
          [](const std::string& name, const glm::vec3& world_position) -> natural_t {
            OTHER_ASSERT(false, "Active scene is null. Should not be accessing scene through lua script with no scene active.");
          }));
    } else {
      CORE_LOG_WARN("Scene native binding table '__other_native' is invalid.");
    }

    get_driver().on_scene_deactivated(active_scene->id);
    active_scene = nullptr;
  }

  void scene_system::push_scene_object_to_context_stack(scene_object* object) {
    PROFILE_SECTION("scene_system::push_scene_object_to_context_stack");
    if (context_stack_top >= kObjectContextStackSize) {
      CORE_LOG_ERROR("Context stack overflow when pushing scene object '{}'", object->name);
      return;
    }

    CORE_LOG_DEBUG("Pushing scene object '{}' to context stack at position {}", object->name, context_stack_top);
    context_stack[context_stack_top++] = object;
  }

  scene_object* scene_system::pop_scene_object_from_context_stack() {
    PROFILE_SECTION("scene_system::pop_scene_object_from_context_stack");
    if (context_stack_top == 0) {
      CORE_LOG_ERROR("Context stack underflow when popping scene object");
      return nullptr;
    }

    CORE_LOG_DEBUG("Popping scene object '{}' from context stack at position {}", context_stack[context_stack_top - 1]->name, context_stack_top - 1);
    return context_stack[--context_stack_top];
  }

  scene_graph& scene_system::get_scene_graph() {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    return *project_scene_graph;
  }

  // void scene_system::handle_scene_load_empty_event(const value& data) {
  //   if (data.type() != value_type::STRING) {
  //     CORE_LOG_ERROR("Invalid data type for force-load-empty-scene event. Expected string.");
  //     return;
  //   }

  //   std::string scene_name = data.as_string();
  //   if (project_scene_graph->has_scene(scene_name)) {
  //     CORE_LOG_WARN("Scene with name [{}] already exists in the scene graph. Cannot force load empty scene with duplicate name.", scene_name);
  //     return;
  //   }

  //   natural_t scene_id = create_empty_scene(scene_name);
  //   set_scene_to_active(scene_id);
  //   OTHER_ASSERT(active_scene != nullptr, "Active scene is null after creating/loading scene.");

  //   CORE_LOG_INFO("Created and loaded empty scene [{}:{}] from console command.", scene_id, scene_name);
  //   constexpr bool is_empty = true;
  //   constexpr bool requires_udp_binding = true;
  //   get_driver().send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
  // }

  void scene_system::register_components() {
    OTHER_ASSERT(component_reg != nullptr, "Component registry is not initialized in scene system.");
    PROFILE_SECTION("scene_system::register_components");

    component_reg->register_component_type<transform>("Transform");
    component_reg->register_component_type<script_component>("Script");
    component_reg->register_component_type<render_component>("Graphics Object");
    component_reg->register_component_type<physics_component>("Physics Object");
    component_reg->register_component_type<physics_joint_component>("Physics Joint");
    component_reg->register_component_type<point_light_component>("Point Light");
    component_reg->register_component_type<direction_light_component>("Directional Light");
    component_reg->register_component_type<camera_component>("Camera");
    component_reg->register_component_type<grid_component>("Grid");
    component_reg->register_component_type<animation_component>("Animation");
    component_reg->register_component_type<audio_source_component>("Audio Source");
    component_reg->register_component_type<audio_listener_component>("Audio Listener");
  }

  void scene_system::handle_scene_load_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_load_event");
    natural_t scene_id = 0;

    if (data.type() == value_type::STRING) {
      std::string scene_path_str = data.as_string();
      filepath scene_path(scene_path_str);
      if (!std::filesystem::exists(scene_path)) {
        CORE_LOG_ERROR("Scene file '{}' does not exist. Cannot load scene.", scene_path.string());
        return;
      }

      CORE_LOG_DEBUG("Loading scene '{}' and adding to scene graph.", scene_path.string());
      scene_id = add_scene_to_scene_graph(scene_path);
    } else if (data.type() == value_type::UINT64) {
      scene_id = data;
    } else {
      CORE_LOG_ERROR("Invalid data type for scene.load-scene event. Expected string (scene path) or uint64 (scene ID).");
      return;
    }

    if (scene_id == 0) {
      CORE_LOG_ERROR("Failed to load scene from file via console command");
      return;
    }

    auto* s = get_scene(scene_id);
    if (s == nullptr) {
      CORE_LOG_ERROR("Scene with ID {} not found in scene graph. Cannot load scene.", scene_id);
      return;
    }

    // this was loaded by command which means user wants the scene now
    s->activate_on_load = true;
    CORE_LOG_DEBUG("Scene loaded with ID {}.", scene_id);
  }

  void scene_system::handle_scene_asset_loaded_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_asset_loaded_event");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data type for scene.asset-loaded event. Expected uint64 (scene ID).");

    natural_t scene_asset_id = data;
    CORE_LOG_DEBUG("Handling scene asset loaded event for scene asset ID {}.", scene_asset_id);

    /// scene.asset_id should have been set in scene pipeline
    CORE_LOG_TRACE("Looking for scene with asset ID {} in scene graph.", scene_asset_id);
    auto* s = project_scene_graph->find_scene([scene_asset_id](const scene& s) {
      CORE_LOG_TRACE(" - scene name: {}, id: {}, asset-id: {}", s.name, s.id, s.asset_id);
      return s.asset_id == scene_asset_id;
    });
    if (s == nullptr && get_driver().get_kernel().has_core_system<asset_system>()) {
      /// resolver-dispatched scene documents are tracked file assets (snapshot nodes)
      //  with no graph scene attached; only assets born through add_scene_asset must
      //  resolve to a graph scene here
      auto& assets = get_driver().get_kernel().get_core_system<asset_system>();
      const asset* scene_asset = assets.get_asset(scene_asset_id);
      if (scene_asset != nullptr && assets.get_asset_manager()->in_snapshot(scene_asset->stable_id)) {
        CORE_LOG_DEBUG("Scene document asset {} ('{}') loaded; no graph scene attached.", scene_asset_id, scene_asset->virtual_path.string());
        return;
      }
    }
    OTHER_ASSERT(s != nullptr, "Scene with asset ID {} not found in scene graph after scene asset loaded event.", scene_asset_id);

    /// this happens here so it only happens once when the asset is fully loaded and registered
    // if project is loading don't check this
    bool try_activate = get_driver().get_kernel().has_core_system<project_system>();
    if (try_activate) {
      auto& project_sys = get_driver().get_kernel().get_core_system<project_system>();
      try_activate = !project_sys.project_loading();
    }

    if (get_driver().get_kernel().has_core_system<project_system>()) {
      auto& project_sys = get_driver().get_kernel().get_core_system<project_system>();
      if (project_sys.project_loading()) {
        project_sys.get_project().add_loaded_scene(s->id);
      }
      /// TODO:
      else {
        CORE_LOG_WARN("Unimplemented handling of scene asset loaded event in project for project state {}", project_sys.get_project().get_state());
      }
    }

    /**
     * \note (is this still relevant?):
     *    - scene must be active to be bound to the native scripting interfaces so we activate it to run the creation script, and then restore the old one.
     *    - we don't want to do any of the other stuff associated with 'primary' activation like triggering events or synchronizing over the network,
     *      so we set the pointer, run the script, and reset it back to the old one before doing the 'real' activation below if needed
     **/

    CORE_LOG_DEBUG("Scene asset loaded: {}", s->name);
    CORE_LOG_DEBUG("try_activate: {}, activate_on_load: {}", try_activate, s->activate_on_load);
    if (try_activate && s->activate_on_load) {
      /// 'real activation'
      set_scene_to_active(s->id);
      synchronize_active_scene(s->id);
    }
  }

  void scene_system::handle_scene_asset_unloaded_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_asset_unloaded_event");
    OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data type for scene.asset-loaded event. Expected uint64 (scene ID).");

    natural_t scene_asset_id = data;
    CORE_LOG_DEBUG("Handling scene asset unloaded event for scene asset ID: {}", scene_asset_id);

    CORE_LOG_TRACE("Looking for scene with asset ID {} in scene graph.", scene_asset_id);
    auto* s = project_scene_graph->find_scene([scene_asset_id](const scene& sc) {
      return sc.asset_id == scene_asset_id;
    });
    if (s == nullptr && get_driver().get_kernel().has_core_system<asset_system>()) {
      /// scene documents (snapshot nodes) unload on refresh/teardown with no graph
      //  scene attached — same contract split as handle_scene_asset_loaded_event
      auto& assets = get_driver().get_kernel().get_core_system<asset_system>();
      const asset* scene_asset = assets.get_asset(scene_asset_id);
      if (scene_asset != nullptr && assets.get_asset_manager()->in_snapshot(scene_asset->stable_id)) {
        CORE_LOG_DEBUG("Scene document asset {} ('{}') unloaded; no graph scene attached.", scene_asset_id, scene_asset->virtual_path.string());
        return;
      }
    }
    OTHER_ASSERT(s != nullptr, "Scene with asset ID '{}' not found in scene graph.", scene_asset_id);

    if (get_driver().get_kernel().has_core_system<project_system>()) {
      auto& project_sys = get_driver().get_kernel().get_core_system<project_system>();
      if (project_sys.is_project_loaded() || project_sys.is_project_unloading()) {
        project_sys.get_project().remove_loaded_scene(s->id);
      }
    }

    project_scene_graph->remove_scene(s->id);
    CORE_LOG_TRACE("Scene with asset ID {} removed from scene graph.", scene_asset_id);
  }

  void scene_system::handle_scene_unload_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_unload_event");
    scene* active_scene = get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to unload.");
      return;
    }
    CORE_LOG_INFO("Unloading active scene '{}'", active_scene->name);
    unload_active_scene();
  }

  void scene_system::handle_scene_info_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_info_event");
    scene* active_scene = get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to get info from.");
      return;
    }

    std::stringstream ss;
    ss << "Active Scene Information:\n";
    ss << "  - Scene ID: " << active_scene->id << "\n";
    ss << "  - Scene Name: " << active_scene->name << "\n";
    ss << "  - Number of Objects: " << active_scene->get_num_objects() << "\n";
    ss << "  - Synchronized: " << (active_scene->synchronized ? "Yes" : "No") << "\n";

    CORE_LOG_INFO("{}", ss.str());
  }

  void scene_system::handle_scene_playback_command_event(const value& data) {
    PROFILE_SECTION("scene_system::handle_scene_playback_command_event");
    scene* active_scene = get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to send playback command to.");
      return;
    }

    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for scene-playback-command event. Expected string.");
      return;
    }

    std::string command = data.as_string();
    if (command == "play") {
      active_scene->play();
      get_driver().on_scene_played(active_scene->id);
    } else if (command == "pause") {
      active_scene->pause();
      get_driver().on_scene_paused(active_scene->id);
    } else if (command == "stop") {
      active_scene->stop();
      get_driver().on_scene_stopped(active_scene->id);
    } else if (command == "debug-physics-on") {
      active_scene->enable_physics_debug_rendering();
    } else if (command == "debug-physics-off") {
      active_scene->disable_physics_debug_rendering();
    } else {
      CORE_LOG_ERROR("Unknown scene playback command '{}'", command);
    }
  }

  void scene_system::handle_ls_scenes_event(driver_kernel* kernel, const value& data) {
    PROFILE_SECTION("scene_system::handle_ls_scenes_event");
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    auto& graph = get_scene_graph();

    std::stringstream ss;
    ss << "Scenes in Scene Graph:\n";
    for (const auto& node : graph) {
      if (node.value == nullptr) {
        continue;
      }
      ss << "  - ID: " << node.value->id << ", Name: " << node.value->name << "\n";
    }

    events->trigger_event("console.output", ss.str());
  }

}  // namespace other