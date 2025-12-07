/**
 * \file ui/console.cpp
 **/
#include "ui/console.hpp"

#include <spdlog/fmt/fmt.h>

#include "ui/console_history_node.hpp"

namespace other {
  namespace ui {

    console_window::console_window(event_system& events)
        : ui_window(events, "Console") {
      auto hist_node = make_scope<console_history_node>(this);
      history_node_id = add_node(std::move(hist_node));
    }

  }  // namespace ui
}  // namespace other