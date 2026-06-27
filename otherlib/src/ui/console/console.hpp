/**
 * \file ui/console/console.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_HPP
#define OTHERLIB_UI_CONSOLE_HPP

#include "core/defines.hpp"

#include "ui/ui_window.hpp"

namespace other {

  class driver;

  namespace ui {

    class console_window : public ui_window {
     public:
      console_window(event_system& events, driver* driver);
      ~console_window() override = default;

     private:
      natural_t history_node_id = 0;
      natural_t input_node_id = 0;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HPP