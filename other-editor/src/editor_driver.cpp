/**
 * \file editor_driver.cpp
 **/
#include "editor_driver.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>

#include "object/camera_component.hpp"
#include "object/scene_object.hpp"
#include "scene/scene.hpp"

#include "ui/object-editor/object_editor.hpp"
#include "ui/scene-hierarchy/scene_hierarchy.hpp"
#include "ui/viewport/viewport.hpp"

// #include "ui/project-creator/.."

namespace other {

  void editor_driver::on_early_initialize() {
    auto& ui = get_ui();
    viewport_id = ui->register_window<ui::viewport>("viewport", context, *get_event_system(), get_renderer(), this);
    ui->register_window<ui::scene_hierarchy>("scene-hierarchy", context, *get_event_system(), this);
    ui->register_window<ui::object_editor>("object-editor", context, *get_event_system(), this);
  }

  void editor_driver::on_initialize() {
    CORE_LOG_INFO("Initialized editor driver.");

    get_event_system()->register_event("viewport.clicked");
    get_event_system()->add_listener("viewport.clicked", [this](const value& data) {
      OTHER_ASSERT(data.type() == value_type::VEC2, "Expected viewport.clicked event data to be of type VEC2 representing the click position.");
      glm::vec2 click_position = data;
      CORE_LOG_INFO("Viewport clicked at position: ({}, {})", click_position.x, click_position.y);
    });

    auto* input_sys = subsystem<input_system>::get();
    OTHER_ASSERT(input_sys != nullptr, "Input system is null");
    input_sys->push_context("editor-controls");

    get_event_system()->add_listener("viewport.resize", [this](const value& val) {
      get_kernel().get_core_system<rendering_system>().handle_viewport_resize_event(val);
    });
  }

  void editor_driver::on_build_driver_input_map(input_map& map) {
    {
      auto* main_ctx = map.find_context("driver-core");
      OTHER_ASSERT(main_ctx != nullptr, "Main context 'driver-core' not found");
    }

    auto& ctx = map.add_context("editor-controls", true);
    ctx.add_action("move", action_value_type::AXIS_2D)
      // keyboard – each key contributes ±1 to one component
      .bind_key(key_code::W, modifier_flags::NONE, 1.f, 1, false)
      .bind_key(key_code::A, modifier_flags::NONE, 1.f, 0, false)
      .bind_key(key_code::S, modifier_flags::NONE, -1.f, 1, false)
      .bind_key(key_code::D, modifier_flags::NONE, -1.f, 0, false)
      // gamepad left stick
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_X, 0.5f, -1.f, 0, false)
      .bind_gamepad_axis(gamepad_axis::LEFT_STICK_Y, 0.5f, -1.f, 1, false);

    ctx.add_action("move_vertical", action_value_type::AXIS_1D)
      .bind_key(key_code::LEFT_SHIFT, modifier_flags::NONE, 1.f)
      .bind_key(key_code::LEFT_CTRL, modifier_flags::NONE, -1.f)
      .bind_gamepad_button(gamepad_button::RIGHT_BUMPER, 1.f)
      .bind_gamepad_button(gamepad_button::LEFT_BUMPER, -1.f);

    ctx.add_action("look", action_value_type::AXIS_2D)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_X, 0.5f, 1.f, 0)
      .bind_gamepad_axis(gamepad_axis::RIGHT_STICK_Y, 0.5f, 1.f, 1);

    ctx.add_action("orbit_hold")
      .bind_mouse_button(mouse_button::MIDDLE);
  }

  void editor_driver::on_begin_frame(render_data* frame_data) {
    if (frame_data == nullptr) {
      return;
    }

    // struct debug_line {
    //   glm::vec3 start;
    //   glm::vec3 end;
    //   glm::vec3 color;
    // };

    // frame_data->debug_data.submit("editor-grid", debug_line{ .start = { -10.f, 0.f, 0.f }, .end = { 10.f, 0.f, 0.f }, .color = { 1.f, 0.f, 0.f, 1.f } });
  }

  void editor_driver::on_viewport_resize(const glm::vec2& size) {
  }

  void editor_driver::update_running() {
    update_input();
  }

  void editor_driver::update_input() {
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

    const bool is_looking_around = input_sys->is_action_pressed("orbit_hold");
    if (!is_looking_around) {
      SDL_SetWindowRelativeMouseMode(subsystem<renderer_backend>::get()->get_main_window(), false);
      return;
    }

    glm::vec2 move = input_sys->get_action_value_2d("move");
    glm::vec2 look = input_sys->get_action_value_2d("look");
    float vertical = input_sys->get_action_value("move_vertical");

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
    } else if (is_looking_around) {
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
  }

}  // namespace other

OTHER_DRIVER(other::editor_driver);