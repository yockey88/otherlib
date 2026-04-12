/**
 * \file driver/systems/event_driver_system.cpp
 **/
#include "driver/systems/event_driver_system.hpp"

#include <string_view>

#include "driver/driver.hpp"

namespace other {

  void event_driver_system::initialize() {
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");

    events->register_event("shutdown-requested");
    events->add_listener("shutdown-requested", [this](const value& data) { get_driver().request_shutdown(); });

    /// open/close ui window events
    events->register_event("open-driver-ui-window");
    events->add_listener("open-driver-ui-window", std::bind_front(&event_driver_system::handle_open_ui_window_event, this));
    events->register_event("close-driver-ui-window");
    events->add_listener("close-driver-ui-window", std::bind_front(&event_driver_system::handle_close_ui_window_event, this));

    // open/close file events
    /// \todo ...

    events->register_event("ls-driver-default");
    events->add_listener("ls-driver-default", std::bind_front(&event_driver_system::handle_list_driver_default_event, this));
    events->register_event("ls-driver-windows");
    events->add_listener("ls-driver-windows", std::bind_front(&event_driver_system::handle_list_driver_windows_event, this));
    events->register_event("ls-driver-files");
    events->add_listener("ls-driver-files", std::bind_front(&event_driver_system::handle_list_driver_files_event, this));
    events->register_event("ls-driver-scenes");
    events->add_listener("ls-driver-scenes", std::bind_front(&event_driver_system::handle_list_driver_scenes_event, this));
    events->register_event("ls-driver-assets");
    events->add_listener("ls-driver-assets", std::bind_front(&event_driver_system::handle_list_driver_assets_event, this));

    // object commands
    events->register_event("object-driver-create");
    events->add_listener("object-driver-create", std::bind_front(&event_driver_system::handle_object_driver_create_event, this));
    events->register_event("object-driver-destroy");
    events->add_listener("object-driver-destroy", std::bind_front(&event_driver_system::handle_object_driver_destroy_event, this));
    events->register_event("object-driver-push");
    events->add_listener("object-driver-push", std::bind_front(&event_driver_system::handle_object_driver_push_event, this));
    events->register_event("object-driver-pop");
    events->add_listener("object-driver-pop", std::bind_front(&event_driver_system::handle_object_driver_pop_event, this));
    events->register_event("object-driver-info");
    events->add_listener("object-driver-info", std::bind_front(&event_driver_system::handle_object_driver_info_event, this));

    /// scene commands
    events->register_event("force-load-empty-scene");
    events->add_listener("force-load-empty-scene", std::bind_front(&event_driver_system::handle_scene_load_empty_event, this));
    events->register_event("force-load-scene");
    events->add_listener("force-load-scene", std::bind_front(&event_driver_system::handle_scene_load_event, this));
    events->register_event("force-unload-scene");
    events->add_listener("force-unload-scene", std::bind_front(&event_driver_system::handle_scene_unload_event, this));
    events->register_event("scene-info-requested");
    events->add_listener("scene-info-requested", std::bind_front(&event_driver_system::handle_scene_info_event, this));
    events->register_event("scene-playback-command");
    events->add_listener("scene-playback-command", std::bind_front(&event_driver_system::handle_scene_playback_command_event, this));
  }

  void event_driver_system::handle_scene_load_empty_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Invalid data type for force-load-empty-scene event. Expected string.");
      return;
    }

    auto& project_scene_graph = get_driver().get_scene_graph();

    std::string scene_name = data.as_string();
    if (project_scene_graph->has_scene(scene_name)) {
      CORE_LOG_WARN("Scene with name [{}] already exists in the scene graph. Cannot force load empty scene with duplicate name.", scene_name);
      return;
    }

    natural_t scene_id = get_driver().create_empty_scene(scene_name);
    get_driver().set_scene_to_active(scene_id);

    scene* active_scene = get_driver().get_active_scene();
    OTHER_ASSERT(active_scene != nullptr, "Active scene is null after creating/loading scene.");

    CORE_LOG_INFO("Created and loaded empty scene [{}:{}] from console command.", scene_id, scene_name);
    constexpr bool is_empty = true;
    constexpr bool requires_udp_binding = true;
    get_driver().send_load_command(active_scene->name, scene_id, is_empty, requires_udp_binding);
  }

  void event_driver_system::handle_scene_load_event(const value& data) {
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
    natural_t scene_id = get_driver().add_scene_to_scene_graph(scene_path);
    if (scene_id == 0) {
      CORE_LOG_ERROR("Failed to load scene from file '{}' via console command.", scene_path.string());
      return;
    }
    CORE_LOG_DEBUG("Scene '{}' loaded with ID {}.", scene_path.string(), scene_id);

    get_driver().set_scene_to_active(scene_id);
    {
      scene* active_scene = get_driver().get_active_scene();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null after loading scene.");
    }

    get_driver().synchronize_active_scene(scene_id);
  }

  void event_driver_system::handle_scene_unload_event(const value& data) {
    scene* active_scene = get_driver().get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to unload.");
      return;
    }
    CORE_LOG_INFO("Unloading active scene '{}'", active_scene->name);
    get_driver().unload_active_scene();
  }

  void event_driver_system::handle_scene_info_event(const value& data) {
    scene* active_scene = get_driver().get_active_scene();
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

  void event_driver_system::handle_scene_playback_command_event(const value& data) {
    scene* active_scene = get_driver().get_active_scene();
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

  void event_driver_system::handle_open_ui_window_event(const value& data) {
    if (data.type() == value_type::STRING) {
      std::string window_type_str = data;
      get_driver().open_ui_window(window_type_str);
    }
    // else if (data.type() == value_type::INT32) {
    //   int32_t window_type_int = data;
    //   if (window_type_int >= 0 && window_type_int < static_cast<int32_t>(driver_ui::NUM_BUILTIN_WINDOW_TYPES)) {
    //     driver_ui_ptr
    //   } else {
    //     CORE_LOG_ERROR("Invalid UI window type index requested to open: {}", window_type_int);
    //   }
    // }
    else {
      CORE_LOG_ERROR("Invalid data type for open-driver-ui-window event: {}", data.type());
      return;
    }
  }

  void event_driver_system::handle_close_ui_window_event(const value& data) {
    if (data.type() == value_type::STRING) {
      std::string window_type_str = data;
      get_driver().close_ui_window(window_type_str);
    }
    // else if (data.type() == value_type::INT32) {
    //   int32_t window_type_int = data;
    //   if (window_type_int >= 0 && window_type_int < static_cast<int32_t>(driver_ui::NUM_BUILTIN_WINDOW_TYPES)) {
    //     close_ui_window(static_cast<driver_ui::builtin_window_type>(window_type_int));
    //   } else {
    //     CORE_LOG_ERROR("Invalid UI window type index requested to close: {}", window_type_int);
    //   }
    // }
    else {
      CORE_LOG_ERROR("Invalid data type for close-driver-ui-window event: {}", data.type());
      return;
    }
  }

  void event_driver_system::handle_list_driver_default_event(const value& data) {
    filepath cwd = std::filesystem::current_path();
    std::stringstream ss;
    ss << "Current Working Directory: " << cwd.string() << "\n";

    for (auto itr = std::filesystem::directory_iterator(cwd); itr != std::filesystem::directory_iterator(); ++itr) {
      ss << " - " << itr->path().filename().string() << (itr->is_directory() ? " [DIR]" : "") << "\n";
    }

    auto& events = get_driver().get_event_system();
    events->trigger_event("console.output", ss.str());
  }

  void event_driver_system::handle_list_driver_windows_event(const value& data) {
    auto& driver_ui_ptr = get_driver().get_ui();
    OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");

    std::vector<std::string> open_windows = driver_ui_ptr->get_open_window_names();
    std::vector<std::string> windows = std::span<const std::string_view>(driver_ui::kBuiltinWindowNames.data(), driver_ui::NUM_BUILTIN_WINDOW_TYPES).subspan(1) |
      std::views::transform([](const std::string_view& name) { return std::string(name); }) |
      std::views::filter([&open_windows](const std::string& name) { return std::ranges::find(open_windows, name) == open_windows.end(); }) |
      std::ranges::to<std::vector>();

    std::stringstream ss;
    ss << "Available Driver UI Windows:\n";
    for (const auto& window_name : open_windows) {
      ss << "  - " << window_name << " (open)\n";
    }
    for (const auto& window_name : windows) {
      ss << "  - " << window_name << "\n";
    }
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
    events->trigger_event("console.output", ss.str());
  }

  void event_driver_system::handle_list_driver_files_event(const value& data) {
  }

  void event_driver_system::handle_list_driver_scenes_event(const value& data) {
  }

  void event_driver_system::handle_list_driver_assets_event(const value& data) {
  }

  void event_driver_system::handle_object_driver_create_event(const value& data) {
  }

  void event_driver_system::handle_object_driver_destroy_event(const value& data) {
  }

  void event_driver_system::handle_object_driver_push_event(const value& data) {
    CORE_LOG_DEBUG("Received object-driver-push event");
    scene* active_scene = get_driver().get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("Error: No active scene to push object from");
      return;
    }

    scene_object* obj = nullptr;
    if (data.type() == value_type::STRING) {
      std::string object_name = data.as_string();
      obj = &active_scene->get_object(object_name);
    } else if (data.type() == value_type::DOUBLE) {
      natural_t id = static_cast<natural_t>((double)data);
      obj = &active_scene->get_object(id);
    }
    if (obj == nullptr) {
      CORE_LOG_ERROR("Error: Failed to find object in active scene to push");
      return;
    }
    get_driver().push_scene_object_to_context_stack(obj);
  }

  void event_driver_system::handle_object_driver_pop_event(const value& data) {
    CORE_LOG_DEBUG("Received object-driver-pop event");
    scene_object* obj = get_driver().pop_scene_object_from_context_stack();
    if (obj == nullptr) {
      CORE_LOG_ERROR("Error: Failed to pop object from context stack");
      return;
    }
  }

  void event_driver_system::handle_object_driver_info_event(const value& data) {
    if (data.type() != value_type::STRING) {
      CORE_LOG_ERROR("Error: Invalid data type for object-driver-info event. Expected string.");
      return;
    }

    std::string str = data;
    std::string info = get_driver().get_driver_info_string(str);
    CORE_LOG_DEBUG("object-driver-info argument: {}", str);
    auto& events = get_driver().get_event_system();
    OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
    events->trigger_event("console.output", info);
  }

}  // namespace other