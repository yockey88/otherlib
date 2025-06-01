/**
 * \file renderer/ui/ui_helpers.cpp
 **/
#include "renderer/ui/ui_helpers.hpp"

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

}  // namespace other