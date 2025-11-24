/**
 * \file ui/console.cpp
 **/
#include "ui/console.hpp"

#include "ui/console_history_node.hpp"

namespace other {
  namespace ui {

    console_window::console_window(event_system& events, console_command_delegate command_delegate)
        : ui_window(events, "Console") {
      auto hist_node = make_scope<console_history_node>(this);
      hist_node->set_command_delegate(command_delegate);
      add_node(std::move(hist_node));
    }

  }  // namespace ui
}  // namespace other