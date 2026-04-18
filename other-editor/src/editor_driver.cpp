/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include "event/event_system.hpp"

#include "object/camera_component.hpp"
#include "object/scene_object.hpp"

#include "driver/systems/scene_system.hpp"
#include "tools/environment_console.hpp"
#include "ui/driver_ui.hpp"

#include "project_window.hpp"
#include "status_window.hpp"

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
    get_event_system()->add_listener("scene.scene-activated", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::UINT64, "Invalid data for 'scene.scene-activated' event. Expected scene ID as number.");
      auto* s = get_active_scene();
      OTHER_ASSERT(s != nullptr, "Active scene is null when handling 'scene.scene-activated' event.");

      natural_t scene_id = data;
      OTHER_ASSERT(s->id == scene_id, "Scene ID in 'scene.scene-activated' event does not match active scene ID. Expected {}, got {}.", s->id, scene_id);

      /// capture camera id
      if (!s->has_object("Camera")) {
        return;
      }

      scene_object& cam_obj = s->get_object("Camera");
      camera_component* cam = s->get_component<camera_component>(&cam_obj);
      OTHER_ASSERT(cam != nullptr, "Camera component is null");
      camera_obj_id = cam_obj.id;
      cam->camera.sensitivity = 10.0f;
    });

    /// \todo load editor pipeline for debug drawing
    get_ui()->register_window<ui::project_window>("Project", *get_event_system());
    get_ui()->register_window<ui::status_window>("Status", *get_event_system(), this);
    get_ui()->open_window("Status");
  }

  void editor_driver::on_build_driver_input_map(input_map& map) {
    {
      auto* main_ctx = map.find_context("driver-core");
      OTHER_ASSERT(main_ctx != nullptr, "Main context 'driver-core' not found");

      main_ctx->add_action("toggle_move_mode")
        .bind_key(key_code::M, modifier_flags::ALT);
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
      .bind_key(key_code::M, modifier_flags::ALT);

    ctx.add_action("orbit_hold")
      .bind_mouse_button(mouse_button::MIDDLE);
  }

  void editor_driver::on_viewport_resize(const glm::vec2& size) {
    auto* active_scene = get_kernel().get_core_system<scene_system>().get_active_scene();
    if (active_scene == nullptr) {
      return;
    }

    scene_object& cam_obj = active_scene->get_object(camera_obj_id);
    camera_component* cam = active_scene->get_component<camera_component>(&cam_obj);
    if (cam != nullptr) {
      // cam->camera.set_viewport_size(size);
    }
  }

  void editor_driver::update_running() {
    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");

    glm::vec2 move = input_sys->get_action_value_2d("move");
    glm::vec2 look = input_sys->get_action_value_2d("look");
    float vertical = input_sys->get_action_value("move_vertical");
    bool is_looking_around = input_sys->is_action_pressed("orbit_hold");

    if (glm::length(move) > 0.01f || glm::abs(vertical) > 0.01f) {
      scene_object& cam_obj = get_kernel().get_core_system<scene_system>().get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_kernel().get_core_system<scene_system>().get_active_scene()->get_component<camera_component>(&cam_obj);
      float speed = 0.1f;

      cam->camera.position += cam->camera.forward() * move.y * speed;
      cam->camera.position += cam->camera.right() * move.x * speed;
      cam->camera.position += cam->camera.up() * vertical * speed;
    }

    if (glm::length(look) > 0.01f) {
      scene_object& cam_obj = get_kernel().get_core_system<scene_system>().get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_kernel().get_core_system<scene_system>().get_active_scene()->get_component<camera_component>(&cam_obj);
      cam->camera.adjust_look_orientation(look.x, look.y);
    }

    if (is_looking_around) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), true);
      glm::vec2 mouse_delta = input_sys->get_mouse_delta();
      scene_object& cam_obj = get_kernel().get_core_system<scene_system>().get_active_scene()->get_object(camera_obj_id);
      camera_component* cam = get_kernel().get_core_system<scene_system>().get_active_scene()->get_component<camera_component>(&cam_obj);
      cam->camera.adjust_look_orientation(mouse_delta.x * 0.1f, mouse_delta.y * 0.1f);
    } else {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
    }
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

OTHER_DRIVER(other::editor_driver);