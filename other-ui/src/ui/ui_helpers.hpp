/**
 * \file renderer/ui_helpers.hpp
 **/
#ifndef OTHERLIB_RENDERER_UI_HELPERS_HPP
#define OTHERLIB_RENDERER_UI_HELPERS_HPP

#include <cstdint>
#include <string>
#include <utility>

#include <imgui/imgui.h>

#include "core/defines.hpp"

namespace other {

  class scoped_id {
   public:
    scoped_id(const scoped_id&) = delete;
    scoped_id& operator=(const scoped_id&) = delete;

    template <typename T>
    scoped_id(T id) {
      ImGui::PushID(id);
    }

    template <>
    scoped_id(const std::string_view id) {
      ImGui::PushID(id.data());
    }

    ~scoped_id() {
      ImGui::PopID();
    }
  };

  class scoped_font {
    bool pushed = false;

   public:
    scoped_font(ImFont* font);
    ~scoped_font();
  };

  class scoped_style {
    bool pushed = false;

   public:
    scoped_style(ImGuiStyleVar var, float new_val);
    scoped_style(ImGuiStyleVar var, const ImVec2& new_col);
    ~scoped_style();
  };

  class scoped_color {
   public:
    scoped_color(ImGuiCol col, uint32_t new_col);
    scoped_color(ImGuiCol col, const ImVec4& new_col);
    ~scoped_color();
  };

  class scoped_color_stack {
    uint32_t count;

    template <typename ColorType, typename... Colors>
    void PushColor(ImGuiCol col, ColorType color, Colors&&... colors) {
      if constexpr (sizeof...(colors) == 0) {
        ImGui::PushStyleColor(col, ImColor(color).Value);
      } else {
        ImGui::PushStyleColor(col, ImColor(color).Value);
        PushColor(std::forward<Colors>(colors)...);
      }
    }

   public:
    scoped_color_stack(const scoped_color_stack&) = delete;
    scoped_color_stack& operator=(const scoped_color_stack&) = delete;

    template <typename ColorType, typename... Colors>
    scoped_color_stack(ImGuiCol first_color, ColorType first, Colors&&... colors)
        : count((sizeof...(colors) / 2) + 1) {
      static_assert(
        (sizeof...(colors) & 1u) == 0,
        "scoped_color_stack requires an even number of arguments");

      PushColor(first_color, first, std::forward<Colors>(colors)...);
    }

    ~scoped_color_stack();
  };

  std::string calculate_display_text(const std::string& text, float max_width);

  void shift_cursor(float x, float y);
  void shift_cursor_x(float x);
  void shift_cursor_y(float y);

  void underline(bool full_width = false, float offx = 0.f, float offy = -1.f);

}  // namespace other

#endif  // OTHERLIB_RENDERER_UI_HELPERS_HPP