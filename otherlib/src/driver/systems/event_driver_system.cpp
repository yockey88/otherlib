/**
 * \file driver/systems/event_driver_system.cpp
 **/
#include "driver/systems/event_driver_system.hpp"

#include <string_view>

#include "driver/driver.hpp"

namespace other {

  void event_driver_system::initialize(driver_kernel* kernel) {
    auto& network = kernel->get_core_system<network_system>();
    event_system_ptr = make_scope<event_system>(network.io_context());

    /// register the core driver events that were previously in driver::initialize
    event_system_ptr->register_event("shutdown-requested");
    event_system_ptr->add_listener("shutdown-requested", [this](const value& data) {
      get_driver().request_shutdown();
    });

    event_system_ptr->register_event("ls-driver-default");
    event_system_ptr->register_event("ls-driver-windows");
    event_system_ptr->register_event("ls-driver-files");
    event_system_ptr->register_event("ls-driver-scenes");
    event_system_ptr->register_event("ls-driver-assets");

    event_system_ptr->register_event("open-driver-ui-window");
    event_system_ptr->register_event("close-driver-ui-window");

    // events->register_event("shutdown-requested");
    // events->add_listener("shutdown-requested", [this](const value& data) { get_driver().request_shutdown(); });

    /// open/close ui window events
    event_system_ptr->add_listener("open-driver-ui-window", [this, kernel](const value& val) { handle_open_ui_window_event(kernel, val); });
    event_system_ptr->add_listener("close-driver-ui-window", [this, kernel](const value& val) { handle_close_ui_window_event(kernel, val); });

    // // open/close file events
    // /// \todo ...

    // events->register_event("ls-driver-default");
    // events->add_listener("ls-driver-default", std::bind_front(&event_driver_system::handle_list_driver_default_event, this));
    // events->register_event("ls-driver-windows");
    // events->add_listener("ls-driver-windows", std::bind_front(&event_driver_system::handle_list_driver_windows_event, this));
    // events->register_event("ls-driver-files");
    // events->add_listener("ls-driver-files", std::bind_front(&event_driver_system::handle_list_driver_files_event, this));
    // events->register_event("ls-driver-scenes");
    // events->add_listener("ls-driver-scenes", std::bind_front(&event_driver_system::handle_list_driver_scenes_event, this));
    // events->register_event("ls-driver-assets");
    // events->add_listener("ls-driver-assets", std::bind_front(&event_driver_system::handle_list_driver_assets_event, this));

    // // object commands
    // events->register_event("object-driver-create");
    // events->add_listener("object-driver-create", std::bind_front(&event_driver_system::handle_object_driver_create_event, this));
    // events->register_event("object-driver-destroy");
    // events->add_listener("object-driver-destroy", std::bind_front(&event_driver_system::handle_object_driver_destroy_event, this));
    // events->register_event("object-driver-push");
    // events->add_listener("object-driver-push", std::bind_front(&event_driver_system::handle_object_driver_push_event, this));
    // events->register_event("object-driver-pop");
    // events->add_listener("object-driver-pop", std::bind_front(&event_driver_system::handle_object_driver_pop_event, this));
    // events->register_event("object-driver-info");
    // events->add_listener("object-driver-info", std::bind_front(&event_driver_system::handle_object_driver_info_event, this));
  }

  void event_driver_system::tick(driver_kernel* kernel, double dt) {
    /// move the SDL_PollEvent loop here from driver::pump_events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          if (!(get_driver().current_driver_state() == driver_state::DRIVER_STATE_SHUTTING_DOWN ||
                get_driver().current_driver_state() == driver_state::DRIVER_STATE_STOPPED)) {
            trigger_event(kernel, "shutdown-requested", {});
          }
          break;
        default: break;
      }

      subsystem<input_system>::get()->process_event(&event);
      if (kernel->has_core_system<rendering_system>()) {
        subsystem<renderer_backend>::get()->handle_event(&event);
      }
    }
  }

  void event_driver_system::shutdown(driver_kernel* kernel) {
    event_system_ptr = nullptr;
  }

  scope<event_system>& event_driver_system::events() {
    OTHER_ASSERT(event_system_ptr != nullptr, "Event system is not initialized in event driver system.");
    return event_system_ptr;
  }

  const scope<event_system>& event_driver_system::events() const {
    OTHER_ASSERT(event_system_ptr != nullptr, "Event system is not initialized in event driver system.");
    return event_system_ptr;
  }

  void event_driver_system::trigger_event(driver_kernel* kernel, const std::string_view name, const value& data) {
    OTHER_ASSERT(event_system_ptr != nullptr, "Event system is not initialized in event driver system.");
    event_system_ptr->trigger_event(name, data);
  }

  void event_driver_system::handle_open_ui_window_event(driver_kernel* kernel, const value& data) {
    if (kernel->has_core_system<rendering_system>() && data.type() == value_type::STRING) {
      auto& rendering_sys = kernel->get_core_system<rendering_system>();

      std::string window_type_str = data;
      rendering_sys.open_ui_window(window_type_str);
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

  void event_driver_system::handle_close_ui_window_event(driver_kernel* kernel, const value& data) {
    if (kernel->has_core_system<rendering_system>() && data.type() == value_type::STRING) {
      auto& rendering_sys = kernel->get_core_system<rendering_system>();

      std::string window_type_str = data;
      rendering_sys.close_ui_window(window_type_str);
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

  // void event_driver_system::handle_list_driver_default_event(driver_kernel* kernel, const value& data) {
  //   filepath cwd = std::filesystem::current_path();
  //   std::stringstream ss;
  //   ss << "Current Working Directory: " << cwd.string() << "\n";

  //   for (auto itr = std::filesystem::directory_iterator(cwd); itr != std::filesystem::directory_iterator(); ++itr) {
  //     ss << " - " << itr->path().filename().string() << (itr->is_directory() ? " [DIR]" : "") << "\n";
  //   }

  //   auto& events = get_driver().get_event_system();
  //   events->trigger_event("console.output", ss.str());
  // }

  // void event_driver_system::handle_list_driver_windows_event(driver_kernel* kernel, const value& data) {
  //   auto& driver_ui_ptr = get_driver().get_ui();
  //   OTHER_ASSERT(driver_ui_ptr != nullptr, "Driver UI is not initialized.");

  //   std::vector<std::string> open_windows = driver_ui_ptr->get_open_window_names();
  //   std::vector<std::string> windows = std::span<const std::string_view>(driver_ui::kBuiltinWindowNames.data(), driver_ui::NUM_BUILTIN_WINDOW_TYPES).subspan(1) |
  //     std::views::transform([](const std::string_view& name) { return std::string(name); }) |
  //     std::views::filter([&open_windows](const std::string& name) { return std::ranges::find(open_windows, name) == open_windows.end(); }) |
  //     std::ranges::to<std::vector>();

  //   std::stringstream ss;
  //   ss << "Available Driver UI Windows:\n";
  //   for (const auto& window_name : open_windows) {
  //     ss << "  - " << window_name << " (open)\n";
  //   }
  //   for (const auto& window_name : windows) {
  //     ss << "  - " << window_name << "\n";
  //   }
  //   auto& events = get_driver().get_event_system();
  //   OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
  //   events->trigger_event("console.output", ss.str());
  // }

  // void event_driver_system::handle_list_driver_files_event(driver_kernel* kernel, const value& data) {
  // }

  // void event_driver_system::handle_list_driver_scenes_event(driver_kernel* kernel, const value& data) {
  // }

  // void event_driver_system::handle_list_driver_assets_event(driver_kernel* kernel, const value& data) {
  // }

  // void event_driver_system::handle_object_driver_create_event(driver_kernel* kernel, const value& data) {
  // }

  // void event_driver_system::handle_object_driver_destroy_event(driver_kernel* kernel, const value& data) {
  // }

  // void event_driver_system::handle_object_driver_push_event(driver_kernel* kernel, const value& data) {
  //   CORE_LOG_DEBUG("Received object-driver-push event");
  //   scene* active_scene = get_driver().get_active_scene();
  //   if (active_scene == nullptr) {
  //     CORE_LOG_ERROR("Error: No active scene to push object from");
  //     return;
  //   }

  //   scene_object* obj = nullptr;
  //   if (data.type() == value_type::STRING) {
  //     std::string object_name = data.as_string();
  //     obj = &active_scene->get_object(object_name);
  //   } else if (data.type() == value_type::DOUBLE) {
  //     natural_t id = static_cast<natural_t>((double)data);
  //     obj = &active_scene->get_object(id);
  //   }
  //   if (obj == nullptr) {
  //     CORE_LOG_ERROR("Error: Failed to find object in active scene to push");
  //     return;
  //   }
  //   get_driver().push_scene_object_to_context_stack(obj);
  // }

  // void event_driver_system::handle_object_driver_pop_event(driver_kernel* kernel, const value& data) {
  //   CORE_LOG_DEBUG("Received object-driver-pop event");
  //   scene_object* obj = get_driver().pop_scene_object_from_context_stack();
  //   if (obj == nullptr) {
  //     CORE_LOG_ERROR("Error: Failed to pop object from context stack");
  //     return;
  //   }
  // }

  // void event_driver_system::handle_object_driver_info_event(driver_kernel* kernel, const value& data) {
  //   if (data.type() != value_type::STRING) {
  //     CORE_LOG_ERROR("Error: Invalid data type for object-driver-info event. Expected string.");
  //     return;
  //   }

  //   std::string str = data;
  //   std::string info = get_driver().get_driver_info_string(str);
  //   CORE_LOG_DEBUG("object-driver-info argument: {}", str);
  //   auto& events = get_driver().get_event_system();
  //   OTHER_ASSERT(events != nullptr, "Event system is not initialized.");
  //   events->trigger_event("console.output", info);
  // }

}  // namespace other