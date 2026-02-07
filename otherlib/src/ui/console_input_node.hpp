/**
 * \file ui/console_input_node.hpp
 **/
#ifndef OTHERLIB_UI_CONSOLE_INPUT_NODE_HPP
#define OTHERLIB_UI_CONSOLE_INPUT_NODE_HPP

#include <functional>
#include <string>
#include <vector>

#include "renderer/ui/ui_node.hpp"

#include "ui/console_widgets.hpp"

namespace other {

  class driver;

  namespace ui {

    struct command_suggestion_provider {
      virtual ~command_suggestion_provider() = default;

      /// given the current input prefix, return matching suggestions
      virtual std::vector<console_w::autocomplete_item>
      get_suggestions(const std::string& prefix) const = 0;
    };

    class console_input_node : public ui_node {
     public:
      console_input_node(ui_window* window, driver* drvr);
      virtual ~console_input_node() = default;

      void set_suggestion_provider(command_suggestion_provider* provider);
      void request_focus();

      using command_handler_fn = std::function<void(const std::string& command)>;
      void set_command_handler(command_handler_fn handler);

     private:
      driver* driver_ptr = nullptr;

      char input_buf[1024] = {};
      bool focus_requested = false;

      static constexpr size_t kMaxHistory = 256;
      std::vector<std::string> history;
      int32_t history_index = -1;  ///< -1 = not navigating history
      std::string saved_input;     ///< preserved input while browsing history

      command_suggestion_provider* suggestion_provider = nullptr;
      std::vector<console_w::autocomplete_item> suggestions;
      int32_t autocomplete_index = -1;
      bool autocomplete_visible = false;

      void navigate_history(int direction);

      void update_suggestions();
      void accept_suggestion();
      void dismiss_autocomplete();

      command_handler_fn command_handler;

      void submit_command();
      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_CONSOLE_INPUT_NODE_HPP