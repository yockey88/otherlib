/**
 * \file ui/console.cpp
 **/
#include "ui/console.hpp"

#include "ui/console_history_node.hpp"
#include "ui/console_input_node.hpp"

#include "imgui.h"

namespace other {
  namespace ui {

    console_window::console_window(event_system& events, driver* driver)
        : ui_window(events, "Console", true) {
      events.register_event("console.trace");
      events.register_event("console.debug");
      events.register_event("console.info");
      events.register_event("console.warn");
      events.register_event("console.error");
      events.register_event("console.critical");
      events.register_event("console.output");
      events.register_event("console.command");
      events.register_event("console.clear");
      events.register_event("console.focus");

      history_node_id = add_node(make_scope<console_history_node>(this, driver));
      input_node_id = add_node(make_scope<console_input_node>(this, driver));
    }

  }  // namespace ui
}  // namespace other