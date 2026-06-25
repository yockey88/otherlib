/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include "object/camera_component.hpp"
#include "object/scene_object.hpp"
#include "scene/scene.hpp"

namespace other {

  void editor_driver::on_initialize() {
    CORE_LOG_INFO("Initialized editor driver.");
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
  }

  void editor_driver::update_running() {
    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");

    auto* scene = get_active_scene();
    if (scene == nullptr) {
      return;
    }

    auto* obj_ptr = scene->find_object_with_tag("main-camera");
    if (obj_ptr == nullptr) {
      return;
    }

    glm::vec2 move = input_sys->get_action_value_2d("move");
    glm::vec2 look = input_sys->get_action_value_2d("look");
    float vertical = input_sys->get_action_value("move_vertical");
    bool is_looking_around = input_sys->is_action_pressed("orbit_hold");

    if (glm::length(move) > 0.01f || glm::abs(vertical) > 0.01f) {
      camera_component* cam = scene->get_component<camera_component>(obj_ptr);
      float speed = 0.1f;

      cam->camera.position += cam->camera.forward() * move.y * speed;
      cam->camera.position += cam->camera.right() * move.x * speed;
      cam->camera.position += cam->camera.up() * vertical * speed;
    }

    if (glm::length(look) > 0.01f) {
      camera_component* cam = scene->get_component<camera_component>(obj_ptr);
      cam->camera.adjust_look_orientation(look.x, look.y);
    }

    if (is_looking_around) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), true);
      glm::vec2 mouse_delta = input_sys->get_mouse_delta();
      camera_component* cam = scene->get_component<camera_component>(obj_ptr);
      cam->camera.adjust_look_orientation(mouse_delta.x * 0.1f, mouse_delta.y * 0.1f);
    } else {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
    }
  }

  input_map editor_driver::get_default_editor_input_map() {
    input_map map;
    map.name = "editor-default";
    map.stick_dead_zone = 0.15f;
    map.trigger_dead_zone = 0.05f;

    {
      auto& ctx = map.add_context("global", /* transparent */ true);

      /// quit / close
      ctx.add_action("quit")
        .bind_key(key_code::Q, modifier_flags::CTRL);

      /// toggle fullscreen
      ctx.add_action("toggle_fullscreen")
        .bind_key(key_code::F11);
    }

    return map;
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