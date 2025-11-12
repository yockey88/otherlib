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

    bool draw_ivec2_control(const std::string& label, glm::ivec2& value, bool& edited, int32_t reset_val = 0, float col_w = 100.f, vector_axis axes = vector_axis::ZERO, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), float speed = 1.f);
    bool edit_ivec2(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::ivec2& value, vector_axis axes, float speed = 1.f, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), const char* format = "%.2f", ImGuiSliderFlags flags = 0);
    bool draw_ivec3_control(const std::string& label, glm::ivec3& value, bool& edited, int32_t reset_val = 0, float col_w = 100.f, vector_axis axes = vector_axis::ZERO, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), float speed = 1.f);
    bool edit_ivec3(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::ivec3& value, vector_axis axes, float speed = 1.f, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), const char* format = "%.2f", ImGuiSliderFlags flags = 0);
    bool draw_ivec4_control(const std::string& label, glm::ivec4& value, bool& edited, int32_t reset_val = 0, float col_w = 100.f, vector_axis axes = vector_axis::ZERO, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), float speed = 1.f);
    bool edit_ivec4(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::ivec4& value, vector_axis axes, float speed = 1.f, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), const char* format = "%.2f", ImGuiSliderFlags flags = 0);

    bool draw_vec2_control(const std::string& label, glm::vec2& value, bool& edited, float reset_val = 0.f, float col_w = 100.f, const glm::vec2& v_min = glm::vec2(-FLT_MAX), const glm::vec2& v_max = glm::vec2(FLT_MAX), float speed = 0.1f);
    bool edit_vec2(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::vec2& value, float speed = 0.1f, const glm::vec2& v_min = glm::vec2(-FLT_MAX), const glm::vec2& v_max = glm::vec2(FLT_MAX), const char* format = "%.2f", ImGuiSliderFlags flags = 0);
    bool draw_vec3_control(const std::string& label, glm::vec3& value, bool& edited, float reset_val = 0.f, float col_w = 100.f, const glm::vec3& v_min = glm::vec3(-FLT_MAX), const glm::vec3& v_max = glm::vec3(FLT_MAX), float speed = 0.1f);
    bool edit_vec3(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::vec3& value, float speed = 0.1f, const glm::vec3& v_min = glm::vec3(-FLT_MAX), const glm::vec3& v_max = glm::vec3(FLT_MAX), const char* format = "%.2f", ImGuiSliderFlags flags = 0);
    bool draw_vec4_control(const std::string& label, glm::vec4& value, bool& edited, float reset_val = 0.f, float col_w = 100.f, const glm::vec4& v_min = glm::vec4(-FLT_MAX), const glm::vec4& v_max = glm::vec4(FLT_MAX), float speed = 0.1f);
    bool edit_vec4(const std::string& label, ImVec2 size, float reset_val, bool& edited, glm::vec4& value, float speed = 0.1f, const glm::vec4& v_min = glm::vec4(-FLT_MAX), const glm::vec4& v_max = glm::vec4(FLT_MAX), const char* format = "%.2f", ImGuiSliderFlags flags = 0);
    bool draw_quat_control(const std::string& label, glm::quat& value, bool& edited, float reset_val = 0.f, float col_w = 100.f, const glm::vec4& v_min = glm::vec4(-1.f), const glm::vec4& v_max = glm::vec4(1.f), float speed = 0.1f);
    bool edit_quat(const char* label, glm::quat& value);

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_UI_WIDGETS_HPP