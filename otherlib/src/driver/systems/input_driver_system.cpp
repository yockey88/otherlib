/**
 * \file driver/systems/input_driver_system.cpp
 **/
#include "driver/systems/input_driver_system.hpp"

#include "input/input_system.hpp"

#include "driver/driver.hpp"

namespace other {

  void input_driver_system::initialize(driver_kernel* kernel) {
    auto* input = subsystem<input_system>::get();
    OTHER_ASSERT(input != nullptr, "Input system subsystem is not initialized.");
    input->load_input_map(get_driver_input_map());
    input->push_context("driver-core");

    input->on_input_change_state([this, kernel](const input_state_change_event& e) { handle_input_event(kernel, e); });
  }

  void input_driver_system::tick(driver_kernel* kernel, double dt) {
  }

  void input_driver_system::shutdown(driver_kernel* kernel) {
  }

  void input_driver_system::handle_input_event(driver_kernel* kernel, const input_state_change_event& event) {
    if (get_driver().rendering_enabled() && event.action_name.starts_with("focus-console-if-open")) {
      auto& events = kernel->get_core_system<event_driver_system>();
      auto& driver_ui_ptr = kernel->get_core_system<rendering_system>().get_driver_ui();
      if (driver_ui_ptr != nullptr && driver_ui_ptr->is_window_open("console")) {
        std::string event_name = "console.focus" + std::string(event.action_name.substr(std::strlen("focus-console-if-open")));
        events.trigger_event(kernel, event_name);
      }
    } else if (event.action_name == "quit") {
      return get_driver().request_shutdown();
    } else {
      return get_driver().on_input_event(event);
    }
  }

  input_map input_driver_system::get_driver_input_map() {
    auto* input = subsystem<input_system>::get();
    OTHER_ASSERT(input != nullptr, "Input system subsystem is not initialized in driver.");

    input_map driver_input_map;
    {
      auto& ctx = driver_input_map.add_context("driver-core", /* transparent = */ false);
      ctx.add_action("quit")
        .bind_key(key_code::Q, modifier_flags::CTRL);
      ctx.add_action("focus-console-if-open")
        .bind_key(key_code::SLASH);
      ctx.add_action("focus-console-if-open-for-command")
        .bind_key(key_code::SEMICOLON, modifier_flags::SHIFT);
    }

    get_driver().on_build_driver_input_map(driver_input_map);
    return driver_input_map;
  }

}  // namespace other