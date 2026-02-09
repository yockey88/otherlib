/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include "event/event_system.hpp"
#include "serialization/reflection.hpp"

#include "physics_world/physics_body.hpp"
#include "script/scripting_environment.hpp"

#include "object/camera_component.hpp"
#include "object/physics_component.hpp"
#include "object/scene_object.hpp"

#include "rendering-pipelines/empty_pipeline.hpp"
#include "tools/environment_console.hpp"
#include "ui/driver_ui.hpp"

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"

namespace other {

  void editor_driver::on_initialize(const command_line& cmd) {
    CORE_LOG_INFO("Initialized editor driver.");

    {
      logger* log = subsystem<logger>::get();
      log->create_logger("other-editor-log", spdlog::level::trace);
      log_sink console_log_sink = {
        .id = logger::get_next_sink_id(),
        .sink_name = "console-sink",
        .sink_pattern = "[%l] %v",
        .level = spdlog::level::info,
        .sink_factory = [&](const config_table& config) -> spdlog::sink_ptr {
          return std::make_shared<console_sink_mt>(get_event_system());
        }
      };

      std::string loggers[] = { "other-editor-log", "other-core-log" };
      log->register_sink(loggers, console_log_sink);
    }

    get_event_system()->register_event("open-project");
    get_event_system()->add_listener("open-project", [this](const value& data) {
      if (data.type() != value_type::STRING) {
        CORE_LOG_ERROR("Invalid data type for open-project event. Expected string.");
        return;
      }

      std::string proj_name = data;
      CORE_LOG_INFO("Opening project: [{}]", proj_name);
    });

    get_event_system()->register_event("edit-project");
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

      camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);
      OTHER_ASSERT(cam != nullptr, "Camera component is null");
      cam->camera.sensitivity = 10.0f;

      // scene_object& my_obj = get_active_scene()->get_object("MyObject");
      // scene_object& floor_obj = get_active_scene()->get_object("Floor");

      // get_active_scene()->add_component<physics_component>(&my_obj, physics_component{ physics_body_settings{ .body_type = BODY_TYPE_DYNAMIC } });
      // get_active_scene()->add_component<physics_component>(&floor_obj, physics_component{ physics_body_settings{ .body_type = BODY_TYPE_STATIC } });
    });

    process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void editor_driver::on_build_driver_input_map(input_map& map) {
    /// not transparent
    {
      auto* main_ctx = map.find_context("driver-core");
      OTHER_ASSERT(main_ctx != nullptr, "Main context 'driver-core' not found");

      main_ctx->add_action("toggle_move_mode")
        .bind_key(key_code::M, modifier_flags::CTRL);
    }

    auto& ctx = map.add_context("editor-camera-controls");

    ctx.add_action("move", action_value_type::AXIS_2D)
      // keyboard – each key contributes ±1 to one component
      .bind_key(key_code::W, modifier_flags::NONE, 1.f, 1)
      .bind_key(key_code::A, modifier_flags::NONE, 1.f, 0)
      .bind_key(key_code::S, modifier_flags::NONE, -1.f, 1)
      .bind_key(key_code::D, modifier_flags::NONE, -1.f, 0)
      // gamepad left stick
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_X, 0.5f, -1.f, 0)
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_Y, 0.5f, -1.f, 1);

    ctx.add_action("move_vertical", action_value_type::AXIS_1D)
      .bind_key(key_code::LEFT_SHIFT, modifier_flags::NONE, 1.f)
      .bind_key(key_code::LEFT_CTRL, modifier_flags::NONE, -1.f)
      .bind_gamepad_button(gamepad_button::RIGHT_BUMPER, 1.f)
      .bind_gamepad_button(gamepad_button::LEFT_BUMPER, -1.f);

    ctx.add_action("look", action_value_type::AXIS_2D)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_X, 0.5f, 1.f, 0)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_Y, 0.5f, 1.f, 1);

    ctx.add_action("toggle_move_mode")
      .bind_key(key_code::M, modifier_flags::CTRL);

    ctx.add_action("orbit_hold")
      .bind_mouse_button(mouse_button::MIDDLE);
  }

  void editor_driver::on_viewport_resize(const glm::vec2& size) {
    auto* active_scene = get_active_scene();
    if (active_scene == nullptr) {
      return;
    }

    scene_object& cam_obj = active_scene->get_object(camera_obj_id);
    camera_component* cam = active_scene->get_component<camera_component>(&cam_obj);
    if (cam != nullptr) {
      // cam->camera.set_viewport_size(size);
    }
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

  void editor_driver::update_running() {
    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");

    glm::vec2 move = input_sys->get_action_value_2d("move");
    float vertical = input_sys->get_action_value("move_vertical");

    if (glm::length(move) > 0.01f || glm::abs(vertical) > 0.01f) {
      scene_object& cam_obj = get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);
      float speed = 0.1f;

      cam->camera.position += cam->camera.forward() * move.y * speed;
      cam->camera.position += cam->camera.right() * move.x * speed;
      cam->camera.position += cam->camera.up() * vertical * speed;
    }

    // gamepad look (right stick)
    glm::vec2 look = input_sys->get_action_value_2d("look");
    if (glm::length(look) > 0.01f) {
      scene_object& cam_obj = get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);
      cam->camera.adjust_look_orientation(look.x, look.y);
    }

    // mouse orbit (middle-click held)
    if (input_sys->is_action_pressed("orbit_hold")) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), true);
      glm::vec2 mouse_delta = input_sys->get_mouse_delta();
      scene_object& cam_obj = get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_active_scene()->get_component<camera_component>(&cam_obj);
      cam->camera.adjust_look_orientation(mouse_delta.x * 0.1f, mouse_delta.y * 0.1f);
    } else {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
    }
  }

  void editor_driver::update_initializing() {
    using namespace std::string_literals;
    // trigger_event("force-load-scene", "resources/scenes/scene1.lua"s);
    // process_driver_event(driver_event::DRIVER_EVENT_READY);
  }

  void editor_driver::on_input_event(const input_state_change_event& event) {
    if (event.action_name == "toggle_move_mode" && event.pressed) {
      auto* input_sys = subsystem<input_system>::get();
      OTHER_ASSERT(input_sys != nullptr, "Input system is null");

      if (auto* active_ctx = input_sys->active_context(); active_ctx != nullptr) {
        if (active_ctx->name == "editor-camera-controls") {
          input_sys->pop_context();
        } else {
          input_sys->push_context("editor-camera-controls");
        }
      }
    }
  }

}  // namespace other