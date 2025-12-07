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

    void console_history_node::on_prepare_render() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::console::kConsoleBackground);
    }

    void console_history_node::on_render_node_body() {
      if (ImGui::BeginChild("ConsoleHistoryScrollRegion", ImVec2(0.f, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushTextWrapPos();

        const auto& history_lines = environment_console::get_console_history();
        for (const auto& line : history_lines) {
          ImGui::SetScrollHereY(1.f);

          auto zoned_time = std::chrono::zoned_time{ std::chrono::current_zone(), line.timestamp };
          std::string time_str = std::format("{:%H:%M:%S}", std::chrono::round<milliseconds>(zoned_time.get_local_time()));

          push_message_color((console_message_type)line.message_type);
          ImGui::Text("[%s] %s", time_str.c_str(), line.input_text.c_str());
          ImGui::PopStyleColor();
        }
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();

      /// input box
      if (ImGui::BeginChild("InputBox", ImVec2(0.f, ImGui::GetFrameHeightWithSpacing()), false)) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::console::kConsoleBackground);

        if (ImGui::InputText("##console_input", environment_console::get_input_buffer(), environment_console::kInputBufferSize, ImGuiInputTextFlags_EnterReturnsTrue)) {
          auto time_point = std::chrono::system_clock::now();
          std::string input_str(environment_console::get_input_buffer());
          environment_console::submit_console_text(input_str, CONSOLE_MESSAGE_MESSAGE, time_point);
          environment_console::clear_input_buffer();
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