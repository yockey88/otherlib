/**
 * \file ui/ui_widgets.hpp
 **/
#ifndef OTHERLIB_UI_UI_WIDGETS_HPP
#define OTHERLIB_UI_UI_WIDGETS_HPP

#include <cstdint>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/value.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_helpers.hpp"


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

    void draw_item_activity_outline(OutlineFlags flags = OutlineFlags_All, ImColor color_highlight = colors::rgba_to_imvec4(colors::kAccent), float rounding = 0.f);
    bool checkbox(const char* label, bool* value);
    void help_marker(const char* desc);

    bool edit_ivec2(const std::string_view label, glm::ivec2& value, const glm::ivec2& min = glm::zero<glm::ivec2>(), const glm::ivec2& max = glm::zero<glm::ivec2>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec2(const std::string_view label, const glm::ivec2& value);
    bool edit_ivec3(const std::string_view label, glm::ivec3& value, const glm::ivec3& min = glm::zero<glm::ivec3>(), const glm::ivec3& max = glm::zero<glm::ivec3>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec3(const std::string_view label, const glm::ivec3& value);
    bool edit_ivec4(const std::string_view label, glm::ivec4& value, const glm::ivec4& min = glm::zero<glm::ivec4>(), const glm::ivec4& max = glm::zero<glm::ivec4>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_ivec4(const std::string_view label, const glm::ivec4& value);

    bool edit_vec2(const std::string_view label, glm::vec2& value, const glm::vec2& min = glm::zero<glm::vec2>(), const glm::vec2& max = glm::zero<glm::vec2>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec2(const std::string_view label, const glm::vec2& value);
    bool edit_vec3(const std::string_view label, glm::vec3& value, const glm::vec3& min = glm::zero<glm::vec3>(), const glm::vec3& max = glm::zero<glm::vec3>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec3(const std::string_view label, const glm::vec3& value);
    bool edit_vec4(const std::string_view label, glm::vec4& value, const glm::vec4& min = glm::zero<glm::vec4>(), const glm::vec4& max = glm::zero<glm::vec4>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_vec4(const std::string_view label, const glm::vec4& value);

    bool edit_quat(const std::string_view label, glm::quat& value, const glm::quat& vmin = glm::quat{ 0.f, 0.f, 0.f, 0.f }, const glm::quat& vmax = glm::quat{ 0.f, 0.f, 0.f, 0.f }, float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_quat(const std::string_view label, const glm::quat& value);

    bool edit_mat3(const std::string_view label, glm::mat3& value, const glm::mat3& vmin = glm::zero<glm::mat3>(), const glm::mat3& vmax = glm::zero<glm::mat3>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_mat3(const std::string_view label, const glm::mat3& value);

    bool edit_mat4(const std::string_view label, glm::mat4& value, const glm::mat4& vmin = glm::zero<glm::mat4>(), const glm::mat4& vmax = glm::zero<glm::mat4>(), float speed = 1.f, ImGuiSliderFlags flags = ImGuiSliderFlags_None);
    void draw_mat4(const std::string_view label, const glm::mat4& value);

    constexpr static inline float kFramePadding = 2.f;
    constexpr static inline float kOutlineSpacing = 1.f;
    constexpr static inline float kLineHeightFactor = 2.f;
    constexpr static inline float kkItemWidthFactor = 0.75f;

    namespace detail {

      template <typename T, bool Const = false>
      bool draw_vector_element(const std::string_view parent_label, const std::string_view label, T& value, const ImVec2& size, const T& vmin, const T& vmax, const ImVec4& colorn, const ImVec4& colorh, const ImVec4& colorp, const ImVec2& button_size, float speed, ImGuiSliderFlags flags) {
        const float outline_spacing = 1.f;

        bool changed = false;

        std::string id_str = std::format("{}:{}", parent_label, label);
        ImGui::PushID(id_str.c_str());

        scoped_style button_frame(ImGuiStyleVar_FramePadding, ImVec2(kFramePadding, 0.f));
        scoped_style button_rounding(ImGuiStyleVar_FrameRounding, 1.f);
        scoped_color_stack button_colors(
          ImGuiCol_Button, colorn,
          ImGuiCol_ButtonHovered, colorh,
          ImGuiCol_ButtonActive, colorp
        );

        const ImGuiIO& io = ImGui::GetIO();
        auto bold_font = io.Fonts->Fonts[0];

        scoped_font font(bold_font);

        const float input_item_w = size.x / 3.f - button_size.x;

        shift_cursor_y(kFramePadding / 2.f);
        std::string button_tag = std::format("{}##{}", label, parent_label);
        if (ImGui::Button(button_tag.c_str(), button_size)) {
          // reset ?
        }

        ImGui::SameLine(0.f, outline_spacing);
        ImGui::SetNextItemWidth(input_item_w);
        shift_cursor_y(-kFramePadding / 2.f);

        value_type element_type = get_value_type<T>();
        ImGuiDataType ig_element_type;
        switch (element_type) {
          case value_type::INT8: ig_element_type = ImGuiDataType_S8; break;
          case value_type::INT16: ig_element_type = ImGuiDataType_S16; break;
          case value_type::INT32: ig_element_type = ImGuiDataType_S32; break;
          case value_type::INT64: ig_element_type = ImGuiDataType_S64; break;
          case value_type::UINT8: ig_element_type = ImGuiDataType_U8; break;
          case value_type::UINT16: ig_element_type = ImGuiDataType_U16; break;
          case value_type::UINT32: ig_element_type = ImGuiDataType_U32; break;
          case value_type::UINT64: ig_element_type = ImGuiDataType_U64; break;
          case value_type::FLOAT: ig_element_type = ImGuiDataType_Float; break;
          case value_type::DOUBLE: ig_element_type = ImGuiDataType_Double; break;
          default:
            OTHER_ASSERT(false, "Unsupported element type in edit_vecn");
        }

        {
          std::string drag_tag = std::format("##{}:{}-drag", parent_label, label);

          ImGui::SetNextItemWidth(input_item_w * kkItemWidthFactor);

          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, ImGui::GetStyle().FramePadding.y + kFramePadding + 1));
          if constexpr (Const) {
            std::string txt = std::format("{}", value);
            ImGui::Text("%s", txt.c_str());
          } else {
            changed = ImGui::DragScalar(drag_tag.c_str(), ig_element_type, &value, speed, &vmin, &vmax, nullptr, flags);
          }
          ImGui::PopStyleVar();
        }

        ImGui::PopID();
        return Const ? false : changed;
      }

      template <natural_t N, typename R, bool Const = false>
      bool draw_elements(const std::string_view parent_label, const std::string_view label, glm::vec<N, R>& value, const ImVec2& size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, const ImVec2& button_size, float speed, ImGuiSliderFlags flags) {
        bool changed = false;
        std::string elem_id = std::format("##{}:{}", parent_label, label);
        changed |= detail::draw_vector_element<R>(elem_id, "X", value.x, size, vmin.x, vmax.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, button_size, speed, flags);
        changed |= detail::draw_vector_element<R>(elem_id, "Y", value.y, size, vmin.y, vmax.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, button_size, speed, flags);
        if constexpr (N >= 3) {
          changed |= detail::draw_vector_element<R>(elem_id, "Z", value.z, size, vmin.z, vmax.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, button_size, speed, flags);
        }
        if constexpr (N == 4) {
          changed |= detail::draw_vector_element<R>(elem_id, "W", value.w, size, vmin.w, vmax.w, ImVec4{ 0.8f, 0.1f, 0.8f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.9f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.8f, 1.f }, button_size, speed, flags);
        }
        return changed;
      }

      bool draw_quat_elements(const std::string_view parent_label, const std::string_view label, glm::quat& value, const ImVec2& size, const glm::quat& vmin, const glm::quat& vmax, const ImVec2& button_size, float speed, ImGuiSliderFlags flags);

      /// \todo evaluate if there needs to be a more generic one with all three template parameters of glm::vec<N, R, Q>
      template <natural_t N, typename R>
      bool edit_vecn(const std::string_view label, glm::vec<N, R>& value, ImVec2 size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, float speed, ImGuiSliderFlags flags) {
        const float spacing = 8.f;

        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
        const ImVec2 button_size = {
          line_height + 2.f,
          line_height
        };

        // ImGui::Text("%s", label.data());

        // /// shift down do y,j,g,etc.. doesn't get cut off by first element
        // shift_cursor_y(kFramePadding + 11.f);
        // underline();

        return detail::draw_elements<N, R>(label, label, value, size, vmin, vmax, button_size, speed, flags);
      }

      template <natural_t N, typename R>
      void draw_vecn(const std::string_view label, const glm::vec<N, R>& value, ImVec2 size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, float speed, ImGuiSliderFlags flags) {
        const float spacing = 8.f;

        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
        const ImVec2 button_size = {
          line_height + 2.f,
          line_height
        };

        // ImGui::Text("%s", label.data());
        // underline();

        glm::vec<N, R> temp = value;
        detail::draw_elements<N, R>(label, label, temp, size, vmin, vmax, button_size, speed, flags);
      }

      bool edit_quat(const std::string_view label, glm::quat& value, ImVec2 size, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags);
      void draw_quat(const std::string_view label, const glm::quat& value, ImVec2 size, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags);

    }  // namespace detail
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_UI_WIDGETS_HPP