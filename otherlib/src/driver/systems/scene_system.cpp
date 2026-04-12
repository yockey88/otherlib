/**
 * \file driver/systems/scene_system.cpp
 **/
#include "driver/systems/scene_system.hpp"

#include "driver/driver.hpp"
#include "scripting/scene_interface.hpp"

namespace other {

  void scene_system::initialize(driver_kernel* kernel) {
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

  void scene_system::tick(driver_kernel* kernel, float dt) {
    // CORE_LOG_INFO("Ticking Scene Driver System with dt = {} seconds.", dt);
  }

  void scene_system::shutdown(driver_kernel* kernel) {
    CORE_LOG_INFO("Shutting down Scene Driver System.");
  }

  void scene_system::handle_scene_load_empty_event(const value& data) {
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

  void scene_system::handle_scene_unload_event(const value& data) {
    scene* active_scene = get_driver().get_active_scene();
    if (active_scene == nullptr) {
      CORE_LOG_ERROR("No active scene to unload.");
      return;
    }
    CORE_LOG_INFO("Unloading active scene '{}'", active_scene->name);
    get_driver().unload_active_scene();
  }

  void scene_system::handle_scene_info_event(const value& data) {
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

  void scene_system::handle_scene_playback_command_event(const value& data) {
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

}  // namespace other