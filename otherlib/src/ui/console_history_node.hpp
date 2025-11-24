/**
 * \file ui/console_history_node.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP
#define OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP

#include <chrono>

#include "renderer/ui/ui_node.hpp"

/// \todo
#include "scripting/actions/action.hpp"

namespace other {
  namespace ui {

    enum console_message_type {
      CONSOLE_MESSAGE_DEBUG = 1 << 0,
      CONSOLE_MESSAGE_INFO = 1 << 1,
      CONSOLE_MESSAGE_WARN = 1 << 2,
      CONSOLE_MESSAGE_ERROR = 1 << 3,
      CONSOLE_MESSAGE_COMMAND = 1 << 4,
    };
    struct console_input {
      std::string input_text = "";
      console_message_type message_type = CONSOLE_MESSAGE_INFO;

      system_timepoint timestamp;
    };
    using console_command_delegate = std::function<bool(const std::string_view, system_timepoint)>;

    class console_history_node : public ui_node {
     public:
      console_history_node(ui_window* parent)
          /// arguments are default except 'true' which means that this node does not start a child region
          : ui_node(parent, "Console History", { 0.f, 0.f }, 0, 0, true) {
      }
      virtual ~console_history_node() = default;

      void set_command_delegate(console_command_delegate delegate) { command_delegate = delegate; }

      void push_message(const console_input& input);

      void on_prepare_render() override;
      void on_render_node_body() override;
      void on_render_end() override;

     private:
      console_command_delegate command_delegate = nullptr;

      std::vector<console_input> history_lines = {};
      size_t max_history_lines = 100;

      constexpr static inline size_t kInputBufferSize = 256;
      std::array<char, kInputBufferSize> input_buffer = { 0 };

      void push_message_color(console_message_type type);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_HISTORY_NODE_HPP