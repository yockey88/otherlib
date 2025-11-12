/**
 * \file ui/ui_widgets.cpp
 **/
#include "ui/ui_widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "renderer/ui/ui_helpers.hpp"

#include "glm/fwd.hpp"

namespace other {
  namespace ui {

    void draw_item_activity_outline(OutlineFlags flags, ImColor color_highlight, float rounding) {
      if (rounding == 0.f) {
        rounding = GImGui->Style.FrameRounding;
      }
      if (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) {
        return;
      }

      auto* draw_list = ImGui::GetWindowDrawList();
      const ImRect rect = rect_expanded(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), 1.f, 1.f);
      if ((flags & OutlineFlags_WhenActive) && ImGui::IsItemActive()) {
        if (flags & OutlineFlags_HighlightActive) {
          draw_list->AddRect(rect.Min, rect.Max, colors::kHighlight, rounding, 0, 1.5f);
        } else {
          draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
      } else if ((flags & OutlineFlags_WhenHovered) && ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(60, 60, 60), rounding, 0, 1.5f);
      } else if ((flags & OutlineFlags_WhenInactive) && !ImGui::IsItemHovered() && !ImGui::IsItemActive()) {
        draw_list->AddRect(rect.Min, rect.Max, ImColor(50, 50, 50), rounding, 0, 1.f);
      }
    }

    bool checkbox(const char* label, bool* value) {
      bool changed = ImGui::Checkbox(label, value);
      draw_item_activity_outline();
      return changed;
    }

    void help_marker(const char* desc) {
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
      }
    }

    constexpr static inline float kFramePadding = 2.f;
    constexpr static inline float kOutlineSpacing = 1.f;
    constexpr static inline float kLineHeightFactor = 2.f;

    bool draw_ivec2_control(const std::string& label, glm::ivec2& value, bool& edited, int32_t reset_val, float col_w, vector_axis axes, const glm::ivec2& min, const glm::ivec2& max, float speed) {
      bool modified = false;

      if (!label.empty()) {
        ImGui::TableSetColumnIndex(0);
        shift_cursor(17.f, 7.f);

        ImGui::Text("%s", label.data());
        underline(false, 0.f, 2.f);
      }

      ImGui::TableSetColumnIndex(1);
      shift_cursor(7.f, 0.f);

      modified = edit_ivec2(label, ImVec2(ImGui::GetContentRegionAvail().x - 8.f, ImGui::GetFrameHeightWithSpacing() + 8.f), reset_val, edited, value, axes, speed, min, max);

      return modified;
    }

    // bool EditVec2(const std::string_view label, ImVec2 size, float reset_val, bool& edited, glm::vec2& value, VectorAxis axes, float speed, const glm::vec2& v_min, const glm::vec2& v_max, const char* format, ImGuiSliderFlags flags) {
    //   ImGui::BeginVertical((std::string(label) + "fr").data());
    //   bool changed = false;
    //   {
    //     const float spacing = 8.f;
    //     ScopedStyle item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
    //     ScopedStyle padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    //     const float frame_padding = 2.f;
    //     const float outline_spacing = 1.f;
    //     const float line_height = GImGui->Font->FontSize + frame_padding * 2.f;
    //     const ImVec2 button_size = {
    //       line_height + 2.f,
    //       line_height
    //     };

    //     const float input_item_w = size.x / 3.f - button_size.x;

    //     ShiftCursorY(frame_padding);

    //     const ImGuiIO& io = ImGui::GetIO();
    //     auto bold_font = io.Fonts->Fonts[0];

    //     auto draw_control = [&](const std::string& label, float& value, const ImVec4& colorn, const ImVec4& colorh, const ImVec4& colorp, bool multi_select, float speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    //       {
    //         ScopedStyle button_frame(ImGuiStyleVar_FramePadding, ImVec2(frame_padding, 0.f));
    //         ScopedStyle button_rounding(ImGuiStyleVar_FrameRounding, 1.f);
    //         ScopedColorStack button_colors(
    //           ImGuiCol_Button, colorn,
    //           ImGuiCol_ButtonHovered, colorh,
    //           ImGuiCol_ButtonActive, colorp
    //         );

    //         ScopedFont font(bold_font);

    //         ShiftCursorY(frame_padding / 2.f);
    //         if (ImGui::Button(label.c_str(), button_size)) {
    //           value = reset_val;
    //           changed = true;
    //         }
    //       }

    //       ImGui::SameLine(0.f, outline_spacing);
    //       ImGui::SetNextItemWidth(input_item_w);

    //       ShiftCursorY(-frame_padding / 2.f);

    //       ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, multi_select);

    //       bool was_temp_input_active = ImGui::TempInputIsActive(ImGui::GetID(("##" + label).c_str()));
    //       changed |= DragFloat(("##" + label).c_str(), &value, speed, v_min, v_max, format, flags);

    //       if (changed && Keyboard::Pressed(Keyboard::Key::OE_TAB)) {
    //         edited = true;
    //       }

    //       if (ImGui::TempInputIsActive(ImGui::GetID(("##" + label).c_str()))) {
    //         changed = false;
    //       }

    //       ImGui::PopItemFlag();

    //       if (was_temp_input_active) {
    //         edited |= ImGui::IsItemDeactivatedAfterEdit();
    //       }
    //     };

    //     draw_control(
    //       "X", value.x,
    //       ImVec4{ 0.8f, 0.1f, 0.15f, 1.f },
    //       ImVec4{ 0.9f, 0.2f, 0.2f, 1.f },
    //       ImVec4{ 0.8f, 0.1f, 0.15f, 1.f },
    //       (axes & VectorAxis::X),
    //       speed, v_min.x, v_max.x, format, flags
    //     );

    //     ImGui::SameLine(0.f, outline_spacing);

    //     draw_control(
    //       "Y", value.y,
    //       ImVec4{ 0.2f, 0.7f, 0.2f, 1.f },
    //       ImVec4{ 0.3f, 0.8f, 0.3f, 1.f },
    //       ImVec4{ 0.2f, 0.7f, 0.2f, 1.f },
    //       (axes & VectorAxis::Y),
    //       speed, v_min.y, v_max.y, format, flags
    //     );

    //     ImGui::SameLine(0.f, outline_spacing);

    //     ImGui::EndVertical();
    //   }
    //   return changed || edited;
    // }

    bool edit_ivec2(const char* label, glm::ivec2& value) {}
    bool edit_ivec3(const char* label, glm::ivec3& value) {}
    bool edit_ivec4(const char* label, glm::ivec4& value) {}

    bool edit_vec2(const char* label, glm::vec2& value) {}
    bool edit_vec3(const char* label, glm::vec3& value) {}
    bool edit_vec4(const char* label, glm::vec4& value) {}
    bool edit_quat(const char* label, glm::quat& value) {}

    bool edit_mat3(const char* label, glm::mat3& value) {}
    bool edit_mat4(const char* label, glm::mat4& value) {}

  }  // namespace ui
}  // namespace other