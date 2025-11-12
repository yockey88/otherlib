/**
 * \file ui/value_ui.hpp
 **/
#ifndef OTHERLIB_UI_VALUE_UI_HPP
#define OTHERLIB_UI_VALUE_UI_HPP

#include <cstdint>

#include <imgui/imgui.h>

#include "core/value.hpp"

namespace other {
  namespace ui {

    void begin_property(const char* name, const char* help_text = "");
    void end_property();

    void begin_property_grid(uint32_t columns = 2, const ImVec2& item_spacing = ImVec2(8.f, 8.f), const ImVec2& frame_pading = ImVec2(4.f, 4.f));
    void end_property_grid();

    void value_editor(const char* label, value& val);

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_VALUE_UI_HPP