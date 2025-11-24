/**
 * \file ui/console_history_node.cpp
 **/
#include "ui/console_history_node.hpp"

#include <chrono>
#include <format>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"

#include "renderer/ui/ui_helpers.hpp"

#include "ui/colors.hpp"

namespace other {
  namespace ui {

    void console_history_node::push_message(const console_input& input) {
      history_lines.push_back(input);
      if (history_lines.size() > max_history_lines) {
        history_lines.erase(history_lines.begin());
      }
    }

    void console_history_node::on_prepare_render() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::console::kConsoleBackground);
    }

    void console_history_node::on_render_node_body() {
      const float line_jump = 2.f * ImGui::GetTextLineHeightWithSpacing();
#if 1
      /// toolbar
      /// \todo

      /// scroll region
      if (ImGui::BeginChild("ConsoleHistoryScrollRegion", ImVec2(0.f, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushTextWrapPos();

        for (const auto& line : history_lines) {
          ImGui::SetScrollHereY(1.f);

          auto zoned_time = std::chrono::zoned_time{ std::chrono::current_zone(), line.timestamp };
          std::string time_str = std::format("{:%H:%M:%S}", std::chrono::round<milliseconds>(zoned_time.get_local_time()));

          push_message_color(line.message_type);
          ImGui::Text("[%s] %s", time_str.c_str(), line.input_text.c_str());
          ImGui::PopStyleColor();
        }
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();

      /// input box
      if (ImGui::BeginChild("InputBox", ImVec2(0.f, ImGui::GetFrameHeightWithSpacing()), false)) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::console::kConsoleBackground);

        if (ImGui::InputText("##console_input", input_buffer.data(), kInputBufferSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
          auto time_point = std::chrono::system_clock::now();
          std::string input_str(input_buffer.data());

          if (input_str.empty()) {
          } else {
            CORE_LOG_TRACE("Processing console input: '{}'", input_str);

            [[maybe_unused]] bool is_cmd = false;
            if (command_delegate != nullptr) {
              is_cmd = command_delegate(input_str, time_point);
            }

            console_message_type msg_type = is_cmd ? CONSOLE_MESSAGE_COMMAND : CONSOLE_MESSAGE_INFO;

            history_lines.push_back({
              .input_text = input_str,
              .message_type = msg_type,
              .timestamp = time_point,
            });
            std::ranges::fill(input_buffer.begin(), input_buffer.end(), 0);
          }
        }

        ImGui::PopStyleColor();
      }
      ImGui::EndChild();
#else
      ImVec2 child_size = ImGui::GetContentRegionAvail();
      ImVec2 start_cursor_pos = ImGui::GetCursorPos();

      /// set cursor to bottom of screen
      float y_pos = start_cursor_pos.y + child_size.y - ImGui::GetTextLineHeightWithSpacing();
      shift_cursor_y(y_pos);

      if (ImGui::InputText("##console_input", input_buffer.data(), kInputBufferSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::string input_str(input_buffer.data());
        if (!input_str.empty()) {
          CORE_LOG_DEBUG("Console input received: {}", input_buffer.data());
          history_lines.push_back(input_str);
        }
        std::ranges::fill(input_buffer.begin(), input_buffer.end(), 0);
      }

      auto itr = history_lines.rbegin();

      /// this first shift makes up for the input box rect which is large then a normal line
      y_pos -= 2.5f * ImGui::GetTextLineHeightWithSpacing();
      shift_cursor_y(-(2.5f * ImGui::GetTextLineHeightWithSpacing()));
      ImGui::TextUnformatted(itr->c_str());
      ++itr;

      for (; itr != history_lines.rend(); ++itr) {
        y_pos -= 2.f * ImGui::GetTextLineHeightWithSpacing();
        shift_cursor_y(-(2.f * ImGui::GetTextLineHeightWithSpacing()));
        if (y_pos <= ImGui::GetWindowContentRegionMin().y) {
          break;
        }

        ImGui::TextUnformatted(itr->c_str());
      }
#endif
    }

    void console_history_node::on_render_end() {
      ImGui::PopStyleColor();
    }

    void console_history_node::push_message_color(console_message_type type) {
      bool is_command = (type & CONSOLE_MESSAGE_COMMAND) != 0;
      if (is_command) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleCommandText);
        return;
      }

      switch (type) {
        case CONSOLE_MESSAGE_DEBUG:
          ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleDebugText);
          break;
        case CONSOLE_MESSAGE_INFO:
          ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleInfoText);
          break;
        case CONSOLE_MESSAGE_WARN:
          ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleWarningText);
          break;
        case CONSOLE_MESSAGE_ERROR:
          ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleErrorText);
          break;
        default:
          ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleOutputText);
          break;
      }
    }

  }  // namespace ui
}  // namespace other