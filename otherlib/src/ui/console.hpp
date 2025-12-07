/**
 * \file ui/console.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_HPP
#define OTHERLIB_UI_CONSOLE_HPP

#include "renderer/ui/ui_window.hpp"

#include "ui/console_history_node.hpp"

namespace other {
  namespace ui {

    class console_window : public ui_window {
     public:
      console_window(event_system& events);
      ~console_window() override = default;

     private:
      natural_t history_node_id = 0;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HPP