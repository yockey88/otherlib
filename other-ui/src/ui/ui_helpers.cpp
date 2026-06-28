/**
 * \file renderer/ui/ui_helpers.cpp
 **/
#include "ui/ui_helpers.hpp"

#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace other {

  scoped_font::scoped_font(ImFont* font) {
    if (font != nullptr) {
      ImGui::PushFont(font);
      pushed = true;
    }
  }

  scoped_font::~scoped_font() {
    if (pushed) {
      ImGui::PopFont();
    }
  }

  scoped_style::scoped_style(ImGuiStyleVar var, float new_val) {
    const ImGuiStyleVarInfo* var_info = ImGui::GetStyleVarInfo(var);
    if (var_info->DataType == ImGuiDataType_Float && var_info->Count == 1) {
      ImGui::PushStyleVar(var, new_val);
      pushed = true;
    }
  }

  scoped_style::scoped_style(ImGuiStyleVar var, const ImVec2& new_col) {
    const ImGuiStyleVarInfo* var_info = ImGui::GetStyleVarInfo(var);
    if (var_info->DataType == ImGuiDataType_Float && var_info->Count == 2) {
      ImGui::PushStyleVar(var, new_col);
      pushed = true;
    }
  }

  scoped_style::~scoped_style() {
    if (pushed) {
      ImGui::PopStyleVar();
    }
  }

  scoped_color::scoped_color(ImGuiCol col, uint32_t new_col) {
    ImGui::PushStyleColor(col, new_col);
  }

  scoped_color::scoped_color(ImGuiCol col, const ImVec4& new_col) {
    ImGui::PushStyleColor(col, new_col);
  }

  scoped_color::~scoped_color() {
    ImGui::PopStyleColor();
  }

  scoped_color_stack::~scoped_color_stack() {
    ImGui::PopStyleColor(count);
  }

  std::string calculate_display_text(const std::string& text, float max_width) {
    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    std::string display_name = text;

    if (text_size.x > max_width) {
      size_t char_fit = static_cast<size_t>(max_width / (text_size.x / text.length()));

      if (char_fit > 3 && char_fit < text.length()) {
        display_name = text.substr(0, char_fit - 3) + "...";
      } else if (char_fit <= 3) {
        display_name = "...";
      }
    }

    return display_name;
  }

  void shift_cursor(float x, float y) {
    const ImVec2 cursor_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cursor_pos.x + x, cursor_pos.y + y));
  }

  void shift_cursor_x(float x) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
  }

  void shift_cursor_y(float y) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y);
  }

  void underline(bool full_width, float offx, float offy) {
    if (full_width) {
      if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr) {
        ImGui::PushColumnsBackground();
      } else if (ImGui::GetCurrentTable() != nullptr) {
        ImGui::TablePushBackgroundChannel();
      }
    }

    const float width = full_width ?
      ImGui::GetWindowWidth() :
      ImGui::GetContentRegionAvail().x;

    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddLine(
      ImVec2(cursor.x + offx, cursor.y + offy),
      ImVec2(cursor.x + width, cursor.y + offy),
      IM_COL32(26, 26, 26, 255) /* dark background */, 1.0f);

    if (full_width) {
      if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr) {
        ImGui::PopColumnsBackground();
      } else if (ImGui::GetCurrentTable() != nullptr) {
        ImGui::TablePopBackgroundChannel();
      }
    }
  }

}  // namespace other