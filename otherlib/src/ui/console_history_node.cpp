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

#include "tools/environment_console.hpp"
#include "ui/colors.hpp"

namespace other {
  namespace ui {

    int text_callback(ImGuiInputTextCallbackData* data) {
      OTHER_ASSERT(data != nullptr, "ImGuiInputTextCallbackData is null");
      OTHER_ASSERT(data->UserData != nullptr, "UserData is null in text_callback, expected console_history_node*");
      console_history_node* console_node = reinterpret_cast<console_history_node*>(data->UserData);
      environment_console::history_move move = environment_console::HISTORY_MOVE_NONE;
      switch (data->EventKey) {
        case ImGuiKey_UpArrow: move = environment_console::HISTORY_MOVE_BACK; break;
        case ImGuiKey_DownArrow: move = environment_console::HISTORY_MOVE_FORWARD; break;
        default:
          break;
      }

      if (move != environment_console::HISTORY_MOVE_NONE) {
        CORE_LOG_DEBUG("text_callback: EventKey = {}, move = {}", data->EventKey, move);
        environment_console::move_history_cursor(move);
        char* input_buffer = environment_console::get_input_buffer();
        OTHER_ASSERT(input_buffer != nullptr, "Input buffer is null in text_callback");

        const auto& history = environment_console::get_console_history();
        size_t cursor = environment_console::get_cursor_position();
        if (cursor < history.size()) {
          std::strncpy(input_buffer, history[cursor].input_text.c_str(), environment_console::kInputBufferSize - 1);
        } else {
          environment_console::clear_input_buffer();
        }
      }

      return 0;
    }

    void console_history_node::on_prepare_render() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::console::kConsoleBackground);
    }

    void console_history_node::on_render_node_body() {
      if (ImGui::BeginChild("ConsoleHistoryScrollRegion", ImVec2(0.f, -ImGui::GetFrameHeightWithSpacing()), false)) {
        ImGui::PushTextWrapPos();
        {
          /// locks console mutex
          const auto history_lines = environment_console::get_console_history();
          for (const auto& line : history_lines) {
            // ImGui::SetScrollHereY(1.f);

            auto zoned_time = std::chrono::zoned_time{ std::chrono::current_zone(), line.timestamp };
            std::string time_str = std::format("{:%H:%M:%S}", std::chrono::round<milliseconds>(zoned_time.get_local_time()));

            push_message_color((console_message_type)line.message_type);
            ImGui::Text("[%s] %s", time_str.c_str(), line.input_text.c_str());
            ImGui::PopStyleColor();
          }
        }
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();

      /// input box
      if (ImGui::BeginChild("InputBox", ImVec2(0.f, ImGui::GetFrameHeightWithSpacing()), false)) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::console::kConsoleBackground);

        bool set_focus = false;
        std::string input = "";

        if (ImGui::IsKeyChordPressed(ImGuiKey_Semicolon | ImGuiKey_ModShift, ImGuiInputFlags_RouteOverActive)) {
          OTHER_ASSERT(std::strlen(environment_console::get_input_buffer()) < environment_console::kInputBufferSize - 2, "Input buffer overflow.");

          environment_console::clear_input_buffer();
          std::strcat(environment_console::get_input_buffer(), ":");

          set_focus = true;
        }

        if (ImGui::InputText("##console_input", environment_console::get_input_buffer(), environment_console::kInputBufferSize, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways, text_callback, (void*)this)) {
          auto time_point = std::chrono::system_clock::now();
          input = std::string(environment_console::get_input_buffer());
          environment_console::submit_console_text(input, CONSOLE_MESSAGE_MESSAGE, time_point);
          environment_console::clear_input_buffer();
        }

        if (set_focus) {
          ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::PopStyleColor();
      }
      ImGui::EndChild();
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

      if ((type & CONSOLE_MESSAGE_ERROR) != 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleErrorText);
        return;
      }

      if ((type & CONSOLE_MESSAGE_WARN) != 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleWarningText);
        return;
      }

      if ((type & CONSOLE_MESSAGE_DEBUG) != 0 || (type & CONSOLE_MESSAGE_INFO) != 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleInfoText);
        return;
      }

      ImGui::PushStyleColor(ImGuiCol_Text, colors::console::kConsoleOutputText);
    }

  }  // namespace ui
}  // namespace other