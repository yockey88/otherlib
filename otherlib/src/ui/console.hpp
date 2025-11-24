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
      console_window(event_system& events, console_command_delegate command_delegate);
      ~console_window() override = default;

     private:
      std::string current_input = "";
      std::vector<std::string> output_lines = {};
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HPP