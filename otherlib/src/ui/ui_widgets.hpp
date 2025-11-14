/**
 * \file ui/ui_widgets.hpp
 **/
#ifndef OTHERLIB_UI_UI_WIDGETS_HPP
#define OTHERLIB_UI_UI_WIDGETS_HPP

#include <cstdint>
#include <string>

#include <glm/gtc/constants.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "ui/colors.hpp"

namespace other {
  namespace ui {

    enum vector_axis : uint32_t {
      ZERO = 0,
      X = 1 << 1,
      Y = 1 << 2,
      Z = 1 << 3,
      W = 1 << 4
    };

    typedef int32_t OutlineFlags;
    enum OutlineFlags_ {
      OutlineFlags_None = 0,
      OutlineFlags_WhenHovered = 1 << 1,
      OutlineFlags_WhenActive = 1 << 2,
      OutlineFlags_WhenInactive = 1 << 3,
      OutlineFlags_HighlightActive = 1 << 4,
      OutlineFlags_NoHighlightActive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive,
      OutlineFlags_NoOutlineInactive = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_HighlightActive,
      OutlineFlags_All = OutlineFlags_WhenHovered | OutlineFlags_WhenActive | OutlineFlags_WhenInactive | OutlineFlags_HighlightActive,
    };

    inline ImRect rect_expanded(const ImRect& rect, float x, float y) {
      return ImRect(
        ImVec2(rect.Min.x - x, rect.Min.y - y),
        ImVec2(rect.Max.x + x, rect.Max.y + y)
      );
    }

    inline ImRect rect_offset(const ImRect& rect, float x, float y) {
      return ImRect(
        ImVec2(rect.Min.x + x, rect.Min.y + y),
        ImVec2(rect.Max.x + x, rect.Max.y + y)
      );
    }

    void draw_item_activity_outline(OutlineFlags flags = OutlineFlags_All, ImColor color_highlight = colors::kAccent, float rounding = 0.f);
    bool checkbox(const char* label, bool* value);
    void help_marker(const char* desc);

    bool edit_ivec2(const std::string_view label, glm::ivec2& value, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec2(const glm::ivec2& value);
    bool edit_ivec3(const std::string_view label, glm::ivec3& value, const glm::ivec3& min = glm::zero<glm::ivec3>(), const glm::ivec3& max = glm::zero<glm::ivec3>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec3(const glm::ivec3& value);
    bool edit_ivec4(const std::string_view label, glm::ivec4& value, const glm::ivec4& min = glm::zero<glm::ivec4>(), const glm::ivec4& max = glm::zero<glm::ivec4>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec4(const glm::ivec4& value);

    bool edit_vec2(const std::string_view label, glm::vec2& value, const glm::vec2& min = glm::zero<glm::vec2>(), const glm::vec2& max = glm::zero<glm::vec2>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec2(const glm::vec2& value);
    bool edit_vec3(const std::string_view label, glm::vec3& value, const glm::vec3& min = glm::zero<glm::vec3>(), const glm::vec3& max = glm::zero<glm::vec3>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec3(const glm::vec3& value);
    bool edit_vec4(const std::string_view label, glm::vec4& value, const glm::vec4& min = glm::zero<glm::vec4>(), const glm::vec4& max = glm::zero<glm::vec4>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec4(const glm::vec4& value);

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_UI_WIDGETS_HPP