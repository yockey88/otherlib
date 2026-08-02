/**
 * \file ui/console/console_input_node.cpp
 **/
#include "ui/console/console_input_node.hpp"

#include "theme/colors.hpp"
#include "tools/environment_console.hpp"

#include "console_history_node.hpp"

namespace other {
  namespace ui {

    console_input_node::console_input_node(ui_window* window, driver* drvr)
        : ui_node(window, "Console Input"), driver_ptr(drvr) {
      /// listen for external focus requests (/ or :)
      events().add_listener("console.focus", [this](const value&) { request_focus(); });
      events().add_listener("console.focus-for-command", [this](const value&) { request_focus(); });
    }

    void console_input_node::set_suggestion_provider(command_suggestion_provider* provider) {
      suggestion_provider = provider;
    }

    void console_input_node::request_focus() {
      focus_requested = true;
    }

    void console_input_node::drop_focus() {
      force_drop_focus = true;
    }

    void console_input_node::navigate_history(int direction) {
      if (history.empty()) return;

      if (history_index == -1) {
        saved_input = input_buf;
      }

      /// add and clamp index
      history_index += direction;
      if (history_index < -1) {
        history_index = -1;
      }
      if (history_index >= static_cast<int32_t>(history.size())) {
        history_index = static_cast<int32_t>(history.size()) - 1;
      }

      if (history_index == -1) {
        /// restore saved input
        std::strncpy(input_buf, saved_input.c_str(), sizeof(input_buf) - 1);
        input_buf[sizeof(input_buf) - 1] = '\0';
      }
      /// history is stored newest-first, index 0 = most recent
      else {
        const std::string& cmd = history[history_index];
        std::strncpy(input_buf, cmd.c_str(), sizeof(input_buf) - 1);
        input_buf[sizeof(input_buf) - 1] = '\0';
      }
    }

    void console_input_node::update_suggestions() {
      if (suggestion_provider == nullptr) {
        suggestions.clear();
        return;
      }

      std::string prefix = input_buf;
      if (prefix.empty()) {
        suggestions.clear();
        autocomplete_visible = false;
        return;
      }

      suggestions = suggestion_provider->get_suggestions(prefix);
      autocomplete_visible = !suggestions.empty();
      autocomplete_index = suggestions.empty() ? -1 : 0;
    }

    void console_input_node::accept_suggestion() {
      if (autocomplete_index < 0 ||
          autocomplete_index >= static_cast<int32_t>(suggestions.size())) {
        return;
      }

      const auto& item = suggestions[autocomplete_index];
      std::strncpy(input_buf, item.label.c_str(), sizeof(input_buf) - 1);
      input_buf[sizeof(input_buf) - 1] = '\0';

      size_t len = std::strlen(input_buf);
      if (len < sizeof(input_buf) - 2) {
        input_buf[len] = ' ';
        input_buf[len + 1] = '\0';
      }

      dismiss_autocomplete();
    }

    void console_input_node::dismiss_autocomplete() {
      autocomplete_visible = false;
      autocomplete_index = -1;
      suggestions.clear();
    }

    void console_input_node::submit_command() {
      std::string command = input_buf;

      /// trim whitespace
      while (!command.empty() && command.back() == ' ') {
        command.pop_back();
      }

      while (!command.empty() && command.front() == ' ') {
        command.erase(command.begin());
      }

      if (command.empty()) {
        return;
      }

      /// add to history (newest first)
      /// avoid duplicating the most recent entry
      if (history.empty() || history.front() != command) {
        history.insert(history.begin(), command);
        if (history.size() > kMaxHistory) {
          history.pop_back();
        }
      }
      history_index = -1;
      saved_input.clear();

      events().trigger_event("console.check-command", value(command));

      /// clear the input
      input_buf[0] = '\0';
      dismiss_autocomplete();
    }

    void console_input_node::on_render_node_body() {
      namespace cw = console_w;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::console::kPromptBG));

      if (!ImGui::BeginChild("##console-input")) {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      /// store position for autocomplete anchor
      /// draw the prompt bar
      ImVec2 prompt_pos = ImGui::GetCursorScreenPos();
      bool was_focus_requested = focus_requested;
      bool was_force_drop_focus = force_drop_focus;
      focus_requested = false;
      force_drop_focus = false;
      if (was_force_drop_focus) {
        ImGui::SetKeyboardFocusHere(-1);  // unfocus
      } else if (was_focus_requested) {
        ImGui::SetKeyboardFocusHere(0);
        /// if the input buffer is empty, prepend ':' for command input
        if (input_buf[0] == '\0') {
          std::strncpy(input_buf, ":", sizeof(input_buf) - 1);
          input_buf[sizeof(input_buf) - 1] = '\0';
        }
      }

      cw::prompt_result pr = cw::draw_prompt_bar(driver_ptr, input_buf, sizeof(input_buf), was_focus_requested);
      if (pr.submitted) {
        if (autocomplete_visible && autocomplete_index >= 0) {
          accept_suggestion();
        } else {
          submit_command();
        }
      }

      if (pr.up_pressed) {
        if (autocomplete_visible) {
          if (autocomplete_index > 0) {
            autocomplete_index--;
          }
        } else {
          navigate_history(1);  // older
        }
      }

      if (pr.down_pressed) {
        if (autocomplete_visible) {
          if (autocomplete_index < static_cast<int32_t>(suggestions.size()) - 1) {
            autocomplete_index++;
          }
        } else {
          navigate_history(-1);  // newer
        }
      }

      if (pr.tab_pressed) {
        if (autocomplete_visible && autocomplete_index >= 0) {
          accept_suggestion();
        } else {
          update_suggestions();
        }
      }

      if (pr.escape_pressed) {
        dismiss_autocomplete();
      }

      /// draw autocomplete popup if active
      if (autocomplete_visible && !suggestions.empty()) {
        cw::autocomplete_result ar = cw::draw_autocomplete_popup(suggestions.data(), static_cast<uint32_t>(suggestions.size()), autocomplete_index, prompt_pos);
        if (ar.confirmed) {
          accept_suggestion();
        }
        if (ar.dismissed) {
          dismiss_autocomplete();
        }
      }

      ImGui::EndChild();
      ImGui::PopStyleColor();
    }

  }  // namespace ui
}  // namespace other