/**
 * \file ui/console_history_node.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP
#define OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP

#include <chrono>

#include "core/timer.hpp"

#include "renderer/ui/ui_node.hpp"

#include "tools/environment_console.hpp"

/// \todo
#include "scripting/actions/action.hpp"

namespace other {
  namespace ui {

    class console_history_node : public ui_node {
     public:
      console_history_node(ui_window* parent)
          /// arguments are default except 'true' which means that this node does not start a child region
          : ui_node(parent, "Console History", { 0.f, 0.f }, 0, 0, true) {
      }
      virtual ~console_history_node() = default;

      void on_prepare_render() override;
      void on_render_node_body() override;
      void on_render_end() override;

     private:
      friend int text_callback(ImGuiInputTextCallbackData* data);

      /// usually points one past last entered input (so input line is empty)
      ///  if user presses up-arrow, cursor moves back to previous entry
      size_t history_cursor = 0;

      void push_message_color(console_message_type type);
    };

  }  // namespace ui

}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP