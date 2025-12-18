/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include "event/event_system.hpp"
#include "serialization/reflection.hpp"

#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"
#include "object/render_component.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"
#include "tools/environment_console.hpp"
#include "ui/driver_ui.hpp"

namespace other {

  void editor_driver::on_initialize(const command_line& cmd) {
    CORE_LOG_INFO("Initialized editor driver.");

    editor_lua_script = subsystem<scripting_environment>::get()->load_lua_file("resources/lua/editor.lua");
    environment_console::initialize(editor_lua_script);
    {
      logger* log = subsystem<logger>::get();
      log->create_logger("other-editor-log", spdlog::level::trace);
      log_sink console_log_sink = {
        .id = logger::get_next_sink_id(),
        .sink_name = "console-sink",
        .sink_pattern = "[%l] %v",
        .level = spdlog::level::info,
        .sink_factory = [](const config_table& config) -> spdlog::sink_ptr {
          return std::make_shared<console_sink_mt>();
        }
      };

      std::string loggers[] = { "other-editor-log", "other-core-log" };
      log->register_sink(loggers, console_log_sink);
    }

    get_event_system()->register_event("open-project");
    get_event_system()->register_event("edit-project");

    get_event_system()->add_listener("open-project", [this](const value& data) {
      if (data.type() != value_type::STRING) {
        CORE_LOG_ERROR("Invalid data type for open-project event. Expected string.");
        return;
      }

      std::string proj_name = data;
      CORE_LOG_INFO("Opening project: [{}]", proj_name);
    });
    get_event_system()->add_listener("edit-project", [this](const value& data) {
      if (data.type() != value_type::STRING) {
        CORE_LOG_ERROR("Invalid data type for edit-project event. Expected string.");
        return;
      }

      std::string proj_name = data;
      CORE_LOG_INFO("Editing project: [{}]", proj_name);
    });
    get_event_system()->add_listener("force-load-scene", [this](const value& data) {
      /// capture camera id
      scene_object& cam_obj = get_active_scene()->get_object("Camera");
      camera_obj_id = cam_obj.id;
    });

    open_ui_window(driver_ui::BUILTIN_WINDOW_CONSOLE);

    /// ready : initializing -> running
    /// nothing to do right now for editor, should start in running state
    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void editor_driver::on_initialize_ui(scope<driver_ui>& ui_ptr) {
    get_event_system()->register_event("editor:main-menu:file:new-project");
    get_event_system()->add_listener("editor:main-menu:file:new-project", [this](const value& data) {
      CORE_LOG_INFO("New Project menu item selected.");
    });

    get_event_system()->register_event("editor:main-menu:file:open-project");
    get_event_system()->add_listener("editor:main-menu:file:open-project", [this](const value& data) {
      CORE_LOG_INFO("Open Project menu item selected.");
    });
  }

  void editor_driver::update_initializing() {
    using namespace std::string_literals;
    // trigger_event("force-load-scene", "resources/scenes/scene1.lua"s);
    // process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void editor_driver::update_running() {
    SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), pressing_mouse_wheel);

    if (pressing_mouse_wheel) {
      /// udpate camera data
      glm::vec2 mouse_pos = get_renderer_instance().get_mouse_position();
      mouse.delta = mouse_pos - mouse.position;
      mouse.position = mouse_pos;

      glm::vec2 rel_pos;
      SDL_GetRelativeMouseState(&rel_pos.x, &rel_pos.y);

      scene_object& cam_obj = get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);
      cam->camera.adjust_look_orientation(rel_pos.x, rel_pos.y);
    }
  }

  static glm::vec4 test_editor_bg_color = glm::vec4(0.03f, 0.03f, 0.03f, 0.7f);
  static float grid_step = 50.0f;
  static float major_grid_step = grid_step * 5.0f;
  static glm::vec4 grid_color = glm::vec4(0.2f, 0.2f, 0.2f, 0.4f);
  static glm::vec4 major_grid_color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
  static float grid_thickness = 1.0f;
  static float major_thickness = 2.0f;

  void editor_driver::on_event(SDL_Event* event) {
    if (get_active_scene() == nullptr) {
      return;
    }

    enum camera_move_flags : uint8_t {
      NONE = 0,
      CAMERA_MOVE_FORWARD = 1 << 0,
      CAMERA_MOVE_BACKWARD = 1 << 1,
      CAMERA_MOVE_RIGHT = 1 << 2,
      CAMERA_MOVE_LEFT = 1 << 3,
      CAMERA_MOVE_UP = 1 << 4,
      CAMERA_MOVE_DOWN = 1 << 5
    };

    uint8_t flags = NONE;
    scene_object& cam_obj = get_active_scene()->get_object(camera_obj_id);
    camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);

    switch (event->type) {
      case SDL_EVENT_KEY_DOWN:
        if (SDLK_W == event->key.key) {
          flags |= CAMERA_MOVE_FORWARD;
        }
        if (SDLK_S == event->key.key) {
          flags |= CAMERA_MOVE_BACKWARD;
        }
        if (SDLK_A == event->key.key) {
          flags |= CAMERA_MOVE_LEFT;
        }
        if (SDLK_D == event->key.key) {
          flags |= CAMERA_MOVE_RIGHT;
        }
        break;

      case SDL_EVENT_MOUSE_WHEEL:
        /// zoom grid in/out
        if (event->wheel.y > 0) {
          grid_step += 1.0f;
          major_grid_step = grid_step * 5.0f;
        } else {
          grid_step = glm::max(1.0f, grid_step - 1.0f);
          major_grid_step = grid_step * 5.0f;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = true;
        }
        break;

      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_MIDDLE) {
          pressing_mouse_wheel = false;
        }
        break;

      default:
        break;
    }

    if (flags == NONE) {
      return;
    }

    if ((flags & CAMERA_MOVE_FORWARD) == CAMERA_MOVE_FORWARD) {
      cam->camera.position += cam->camera.forward() * cam->camera.sensitivity;
    }

    if ((flags & CAMERA_MOVE_BACKWARD) == CAMERA_MOVE_BACKWARD) {
      cam->camera.position -= cam->camera.forward() * cam->camera.sensitivity;
    }

    if ((flags & CAMERA_MOVE_RIGHT) == CAMERA_MOVE_RIGHT) {
      cam->camera.position += cam->camera.right() * cam->camera.sensitivity;
    }

    if ((flags & CAMERA_MOVE_LEFT) == CAMERA_MOVE_LEFT) {
      cam->camera.position -= cam->camera.right() * cam->camera.sensitivity;
    }
  }

}  // namespace other