/**
 * \file ui_theme_settings.cpp
 **/
#include "theme/ui_theme_settings.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace other {

  void ui_theme_settings::apply(ImGuiStyle* style) const {
    if (style == nullptr) {
      return;
    }

    style->ItemSpacing = ImVec2(item_spacing.x * density, item_spacing.y * density);
    style->FramePadding = ImVec2(frame_padding.x * density, frame_padding.y * density);
    style->CellPadding = ImVec2(cell_padding.x * density, cell_padding.y * density);
    style->IndentSpacing = indent_spacing * density;
    style->ScrollbarSize = scrollbar_size * density;
    style->GrabMinSize = grab_min_size * density;
    style->WindowRounding = style->FrameRounding = style->PopupRounding = rounding * density;
    style->TabRounding = rounding * density;
    style->WindowBorderSize = style->FrameBorderSize = border_size * density;
  }

}  // namespace other