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

    events->register_event("force-load-empty-scene");
    events->add_listener("force-load-empty-scene", std::bind_front(&scene_system::handle_scene_load_empty_event, this));
    events->register_event("force-load-scene");
    events->add_listener("force-load-scene", std::bind_front(&scene_system::handle_scene_load_event, this));
    events->register_event("force-unload-scene");
    events->add_listener("force-unload-scene", std::bind_front(&scene_system::handle_scene_unload_event, this));
    events->register_event("scene-info-requested");
    events->add_listener("scene-info-requested", std::bind_front(&scene_system::handle_scene_info_event, this));
    events->register_event("scene-playback-command");
    events->add_listener("scene-playback-command", std::bind_front(&scene_system::handle_scene_playback_command_event, this));

    scene_interface::initialize(&get_driver());
  }

  void scene_system::tick(driver_kernel* kernel, double dt) {
    if (scene* active_scene = get_active_scene(); active_scene != nullptr) {
      active_scene->update(dt);
      active_scene->late_update(dt);
    }
  }

  void scene_system::shutdown(driver_kernel* kernel) {
  }

  void scene_system::new_blank_scene(const std::string_view name) {
    if (project_scene_graph->has_scene(name)) {
      CORE_LOG_WARN("Scene with name '{}' already exists, cannot create new blank scene with duplicate name.", name);
      return;
    }

    if (active_scene != nullptr) {
      CORE_LOG_DEBUG("Unloading current scene [{}:{}] before creating new blank scene.", active_scene->id, active_scene->name);
      unload_active_scene();
    }

    set_scene_to_active(create_new_scene(name));
  }

  natural_t scene_system::create_new_scene(const std::string_view name) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    auto [scene_id, ptr] = project_scene_graph->create_new_scene(name);
    OTHER_ASSERT(ptr != nullptr, "Failed to create new scene: {}", name);
    CORE_LOG_INFO("Created new scene [{}:{}]", scene_id, name);
    return scene_id;
  }

  natural_t scene_system::add_scene_to_scene_graph(const filepath& scene_path) {
    OTHER_ASSERT(project_scene_graph != nullptr, "Project scene graph is not initialized.");
    auto [id, scene_ptr] = project_scene_graph->load_scene(scene_path);
    if (scene_ptr == nullptr) {
      return 0;
    }
    return id;
  }

  natural_t scene_system::create_empty_scene(const std::string_view name) {
    auto [id, _] = project_scene_graph->create_new_scene(name);
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
    CORE_LOG_DEBUG("Setting scene [{}] as active scene in driver.", scene_id);
    if (active_scene != nullptr && active_scene->id == scene_id) {
      CORE_LOG_WARN("Scene [{}] is already the active scene.", scene_id);
      return;
    }

    if (active_scene != nullptr) {
      CORE_LOG_DEBUG("Another scene [{}:{}] is already active, unloading it first.", active_scene->id, active_scene->name);
      unload_active_scene();
    }

    active_scene = project_scene_graph->get_scene(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Scene with ID {} not found in scene graph.", scene_id);
    CORE_LOG_DEBUG("Scene [{}:{}] Activation.", active_scene->id, active_scene->name);

    if (active_scene->script_path.has_value() &&
        /// we can deactivate and reactivate scenes and we don't want to reload the script right now.
        /// maybe in the future we will want to
        !active_scene->script_loaded) {
      CORE_LOG_DEBUG("Active scene has script path '{}', loading scene script.", active_scene->script_path->string());
      active_scene->run_script_file();
    }

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
      active_scene->play();
    }
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

  void scene_system::handle_scene_load_empty_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for force-load-empty-scene event. Expected string.");
      return;
    }

    std::string scene_name = data.as_string();
    if (project_scene_graph->has_scene(scene_name)) {
      CORE_LOG_WARN("Scene with name [{}] already exists in the scene graph. Cannot force load empty scene with duplicate name.", scene_name);
      return;
    }

    natural_t scene_id = create_empty_scene(scene_name);
    set_scene_to_active(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Active scene is null after creating/loading scene.");

    CORE_LOG_INFO("Created and loaded empty scene [{}:{}] from console command.", scene_id, scene_name);
    constexpr bool is_empty = true;
    constexpr bool requires_udp_binding = true;
    get_driver().send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
  }

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

    set_scene_to_active(scene_id);
    OTHER_ASSERT(active_scene != nullptr, "Active scene is null after loading scene.");

    synchronize_active_scene(scene_id);
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
    // ss << "  - Number of Objects: " << active_scene->get_num_objects() << "\n";
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
      CORE_LOG_INFO("Starting scene '{}'", active_scene->name);
      active_scene->play();
    } else if (command == "pause") {
      CORE_LOG_INFO("Pausing scene '{}'", active_scene->name);
      active_scene->stop();
    } else if (command == "stop") {
      CORE_LOG_INFO("Stopping scene '{}'", active_scene->name);
      active_scene->stop();
      active_scene->reset();
    } else {
      CORE_LOG_ERROR("Unknown scene playback command '{}'", command);
    }
  }

}  // namespace other