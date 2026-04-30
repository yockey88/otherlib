/**
 * \file driver/systems/scene_system.cpp
 **/
#include "driver/systems/scene_system.hpp"

#include "driver/driver.hpp"
#include "scripting/scene_interface.hpp"

namespace other {

  void scene_system::initialize(driver_kernel* kernel) {
    project_scene_graph = make_scope<scene_graph>();
    OTHER_ASSERT(project_scene_graph != nullptr, "Failed to create project scene graph.");

    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    events->register_event("scene.load-scene");
    events->add_listener("scene.load-scene", std::bind_front(&scene_system::handle_scene_load_event, this));
    events->register_event("scene.asset-loaded");
    events->add_listener("scene.asset-loaded", std::bind_front(&scene_system::handle_scene_asset_loaded_event, this));

    events->register_event("scene.unload-scene");
    events->add_listener("scene.unload-scene", std::bind_front(&scene_system::handle_scene_unload_event, this));
    events->register_event("scene.request-info");
    events->add_listener("scene.request-info", std::bind_front(&scene_system::handle_scene_info_event, this));
    events->register_event("scene.playback-command");
    events->add_listener("scene.playback-command", std::bind_front(&scene_system::handle_scene_playback_command_event, this));

    events->register_event("scene.scene-activated");

    events->register_event("ls.scenes");
    events->add_listener("ls.scenes", [this](const value& data) { handle_ls_scenes_event(&get_driver().get_kernel(), data); });

    scene_interface::initialize(&get_driver());
  }

  void scene_system::tick(driver_kernel* kernel, double dt) {
    if (scene* active_scene = get_active_scene(); active_scene != nullptr) {
      active_scene->update(dt);
      active_scene->late_update(dt);
    }
  }

  void scene_system::shutdown(driver_kernel* kernel) {
    if (active_scene != nullptr) {
      unload_active_scene();
    }
    project_scene_graph = nullptr;
  }

  void scene_system::load_project_scene_graph(const project& p) {
    struct scene_info {
      std::string name;
      filepath path;
      std::vector<std::string> incoming;
      std::vector<std::string> outgoing;
    };
    std::vector<scene_info> scenes_to_load;
  }

  natural_t scene_system::add_scene_to_scene_graph(const filepath& scene_path) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");

    if (project_scene_graph->has_scene(scene_path.filename().stem().string())) {
      return project_scene_graph->get_scene(scene_path.filename().stem().string())->id;
    }

    auto id = create_empty_scene(scene_path.filename().stem().string(), false);
    OTHER_ASSERT(id != 0, "Failed to add scene [{}] to scene graph.", scene_path.string());
    CORE_LOG_DEBUG("Added scene [{}] to scene graph with ID {}", scene_path.string(), id);

    auto* s = get_scene(id);
    OTHER_ASSERT(s != nullptr, "Failed to retrieve scene [{}] after adding to scene graph.", scene_path.string());
    s->script_path = scene_path;

    get_driver().add_scene_asset(s, scene_path);
    return id;
  }

  natural_t scene_system::create_empty_scene(const std::string_view name, bool add_asset) {
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
    return project_scene_graph->get_scene(id);
  }

  void scene_system::set_scene_to_active(natural_t scene_id) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    if (active_scene != nullptr && active_scene->id == scene_id) {
      return;
    }

    if (active_scene != nullptr) {
      CORE_LOG_DEBUG("Another scene [{}:{}] is already active, unloading it first.", active_scene->id, active_scene->name);
      unload_active_scene();
    }
    CORE_LOG_DEBUG("Setting scene [{}] to active.", scene_id);

    active_scene = project_scene_graph->get_scene(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Scene with ID {} not found in scene graph.", scene_id);
    CORE_LOG_DEBUG("Scene [{}:{}] Activation.", active_scene->id, active_scene->name);

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

    /// 60 fps fixed update
    /// \todo make fixed update time configurable
    if (get_driver().get_event_system()->has_event("scene-update")) {
      get_driver().get_event_system()->cancel_event("scene-update");
    }

    get_driver().get_event_system()->register_timed_event("scene-update", milliseconds(16), true);
    get_driver().get_event_system()->add_listener("scene-update", [this](const value& data) {
      OTHER_ASSERT(active_scene != nullptr, "No active scene in driver during scene update event.");
      constexpr static float kSixtyHertzFixedDeltaTime = 1.0f / 60.0f;
      active_scene->fixed_update(kSixtyHertzFixedDeltaTime);
    });

    if (get_driver().should_auto_play_scenes()) {
      CORE_LOG_DEBUG("Auto-playing scene [{}:{}] on activation.", active_scene->id, active_scene->name);
      active_scene->play();
    }

    auto& events = get_driver().get_event_system();
    events->trigger_event("scene.scene-activated", active_scene->id);
  }

  void scene_system::synchronize_active_scene(natural_t scene_id) {
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
    if (active_scene == nullptr) {
      CORE_LOG_WARN("No active scene to unload in driver.");
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

    get_driver().get_event_system()->cancel_event("scene-update");

    /// \todo decide whether to actually unload or to leaved cached, for now just stop it and
    ///        and leave in the graph, but not active
    // project_scene_graph->remove_scene(active_scene->id);
    active_scene = nullptr;
  }

  void scene_system::push_scene_object_to_context_stack(scene_object* object) {
    if (context_stack_top >= kObjectContextStackSize) {
      CORE_LOG_ERROR("Context stack overflow when pushing scene object '{}'", object->name);
      return;
    }

    CORE_LOG_DEBUG("Pushing scene object '{}' to context stack at position {}", object->name, context_stack_top);
    context_stack[context_stack_top++] = object;
    get_driver().on_push_scene_object(context_stack[context_stack_top - 1]);
  }

  scene_object* scene_system::pop_scene_object_from_context_stack() {
    if (context_stack_top == 0) {
      CORE_LOG_ERROR("Context stack underflow when popping scene object");
      return nullptr;
    }

    CORE_LOG_DEBUG("Popping scene object '{}' from context stack at position {}", context_stack[context_stack_top - 1]->name, context_stack_top - 1);
    get_driver().on_pop_scene_object(context_stack[context_stack_top - 1]);
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

  void scene_system::handle_scene_load_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for load-scene event. Expected string.");
      return;
    }

    std::string scene_path_str = data.as_string();
    filepath scene_path(scene_path_str);
    if (!std::filesystem::exists(scene_path)) {
      CORE_LOG_ERROR("Scene file '{}' does not exist. Cannot load scene.", scene_path.string());
      return;
    }

    CORE_LOG_DEBUG("Loading scene '{}' and adding to scene graph.", scene_path.string());
    natural_t scene_id = add_scene_to_scene_graph(scene_path);
    if (scene_id == 0) {
      CORE_LOG_ERROR("Failed to load scene from file '{}' via console command.", scene_path.string());
      return;
    }
    CORE_LOG_DEBUG("Scene '{}' loaded with ID {}.", scene_path.string(), scene_id);

    auto* s = get_scene(scene_id);
    OTHER_ASSERT(s != nullptr, "Scene with ID {} not found in scene graph after loading scene.", scene_id);
    s->activate_on_load = true;
  }

  void scene_system::handle_scene_asset_loaded_event(const value& data) {
    OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data type for scene.asset-loaded event. Expected uint64 (scene ID).");
    natural_t scene_asset_id = data;
    CORE_LOG_DEBUG("Handling scene asset loaded event for scene asset ID {}.", scene_asset_id);

    auto* s = project_scene_graph->find_scene([scene_asset_id](const scene& s) { return s.asset_id == scene_asset_id; });
    OTHER_ASSERT(s != nullptr, "Scene with asset ID {} not found in scene graph after scene asset loaded event.", scene_asset_id);

    /**
     * \note:
     *    - scene must be active to be bound to the native scripting interfaces so we activate it to run the creation script, and then restore the old one.
     *    - we don't want to do any of the other stuff associated with 'primary' activation like triggering events or synchronizing over the network,
     *      so we set the pointer, run the script, and reset it back to the old one before doing the 'real' activation below if needed
     **/
    {
      scene* curr_active = active_scene;
      active_scene = s;
      s->run_script_file();
      active_scene = curr_active;
    }

    /// this happens here so it only happens once when the asset is fully loaded and registered
    if (s->activate_on_load) {
      /// 'real activation'
      set_scene_to_active(s->id);
      synchronize_active_scene(s->id);
    }
  }

  void scene_system::handle_scene_unload_event(const value& data) {
    scene* active_scene = get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to unload.");
      return;
    }
    CORE_LOG_INFO("Unloading active scene '{}'", active_scene->name);
    unload_active_scene();
  }

  void scene_system::handle_scene_info_event(const value& data) {
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
    } else if (command == "pause") {
      active_scene->pause();
    } else if (command == "stop") {
      active_scene->stop();
    } else {
      CORE_LOG_ERROR("Unknown scene playback command '{}'", command);
    }
  }

  void scene_system::handle_ls_scenes_event(driver_kernel* kernel, const value& data) {
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    auto& graph = get_scene_graph();

    std::stringstream ss;
    ss << "Scenes in Scene Graph:\n";
    for (const auto& node : graph) {
      ss << "  - ID: " << node.value.id << ", Name: " << node.value.name << "\n";
    }

    events->trigger_event("console.output", ss.str());
  }

}  // namespace other