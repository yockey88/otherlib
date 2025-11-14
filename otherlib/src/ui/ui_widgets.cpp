/**
 * \file ui/ui_widgets.cpp
 **/
#include "ui/ui_widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/logger.hpp"

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
    constexpr static inline float kkItemWidthFactor = 0.75f;

    namespace detail {
      template <typename T, bool Const = false>
      bool draw_vector_element(const std::string_view parent_label, const std::string_view label, T& value, const ImVec2& size, const T& vmin, const T& vmax, const ImVec4& colorn, const ImVec4& colorh, const ImVec4& colorp, const ImVec2& button_size, float speed, ImGuiSliderFlags flags) {
        const float outline_spacing = 1.f;

        bool changed = false;

        ImGui::PushID(std::format("{}:{}", parent_label, label).c_str());

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
          float original_frame_padding_y = ImGui::GetStyle().FramePadding.y;
          ImGui::GetStyle().FramePadding.y += kFramePadding + 1;

          std::string drag_tag = std::format("##{}:{}-drag", parent_label, label);

          ImGui::SetNextItemWidth(input_item_w * kkItemWidthFactor);
          if constexpr (Const) {
            std::string txt = std::format("{}", value);
            ImGui::Text("%s", txt.c_str());
          } else {
            changed |= ImGui::DragScalar(drag_tag.c_str(), ig_element_type, &value, speed, &vmin, &vmax, nullptr, flags);  // && ImGui::IsKeyDown(ImGuiKey_Tab);
          }
          changed = !ImGui::TempInputIsActive(ImGui::GetID(drag_tag.c_str()));

          ImGui::GetStyle().FramePadding.y = original_frame_padding_y;
        }

        ImGui::PopID();
        if constexpr (Const) {
          return false;
        } else {
          return changed;
        }
      }

      template <natural_t N, typename R, bool Const = false>
      bool draw_elements(const std::string_view parent_label, const std::string_view label, glm::vec<N, R>& value, const ImVec2& size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, const ImVec2& button_size, float speed, ImGuiSliderFlags flags) {
        bool changed = false;

        changed |= detail::draw_vector_element<R>(
          std::format("##{}:{}", parent_label, label),
          "X", value.x, size, vmin.x, vmax.x,
          ImVec4{ 0.8f, 0.1f, 0.15f, 1.f },
          ImVec4{ 0.9f, 0.2f, 0.2f, 1.f },
          ImVec4{ 0.8f, 0.1f, 0.15f, 1.f },
          button_size, speed, flags
        );
        changed |= detail::draw_vector_element<R>(
          std::format("##{}:{}", parent_label, label),
          "Y", value.y, size, vmin.y, vmax.y,
          ImVec4{ 0.2f, 0.7f, 0.2f, 1.f },
          ImVec4{ 0.3f, 0.8f, 0.3f, 1.f },
          ImVec4{ 0.2f, 0.7f, 0.2f, 1.f },
          button_size, speed, flags
        );

        if constexpr (N >= 3) {
          changed |= detail::draw_vector_element<R>(
            std::format("##{}:{}", parent_label, label),
            "Z", value.z, size, vmin.z, vmax.z,
            ImVec4{ 0.1f, 0.25f, 0.8f, 1.f },
            ImVec4{ 0.2f, 0.35f, 0.9f, 1.f },
            ImVec4{ 0.1f, 0.25f, 0.8f, 1.f },
            button_size, speed, flags
          );
        }

        if constexpr (N == 4) {
          changed |= detail::draw_vector_element<R>(
            std::format("##{}:{}", parent_label, label),
            "W", value.w, size, vmin.w, vmax.w,
            ImVec4{ 0.8f, 0.1f, 0.8f, 1.f },
            ImVec4{ 0.9f, 0.2f, 0.9f, 1.f },
            ImVec4{ 0.8f, 0.1f, 0.8f, 1.f },
            button_size, speed, flags
          );
        }
        return changed;
      }

      /// \todo evaluate if there needs to be a more generic one with all three template parameters of glm::vec<N, R, Q>
      template <natural_t N, typename R>
      bool edit_vecn(const std::string_view label, glm::vec<N, R>& value, ImVec2 size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, float speed, ImGuiSliderFlags flags) {
        // ImGui::BeginVertical((std::string(label) + "fr").data());
        const float spacing = 8.f;

        bool changed = false;
        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
        const ImVec2 button_size = {
          line_height + 2.f,
          line_height
        };

        ImGui::Text("%s", label.data());
        underline();

        return detail::draw_elements<N, R>(label, label, value, size, vmin, vmax, button_size, speed, flags);
      }

      template <natural_t N, typename R>
      void edit_vecn(const std::string_view label, const glm::vec<N, R>& value, ImVec2 size, const glm::vec<N, R>& vmin, const glm::vec<N, R>& vmax, float speed, ImGuiSliderFlags flags) {
        const float spacing = 8.f;

        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
        const ImVec2 button_size = {
          line_height + 2.f,
          line_height
        };

        ImGui::Text("%s", label.data());
        underline();

        glm::vec<N, R> temp = value;
        detail::draw_elements<N, R>(label, label, temp, size, vmin, vmax, button_size, speed, flags);
      }

    }  // namespace detail

    bool edit_ivec2(const std::string_view label, glm::ivec2& value, const glm::ivec2& min, const glm::ivec2& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<2, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec2(const glm::ivec2& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec2##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<2, int32_t>(label.data(), value, size, glm::zero<glm::ivec2>(), glm::zero<glm::ivec2>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_ivec3(const std::string_view label, glm::ivec3& value, const glm::ivec3& min, const glm::ivec3& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<3, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec3(const glm::ivec3& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec3##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<3, int32_t>(label.data(), value, size, glm::zero<glm::ivec3>(), glm::zero<glm::ivec3>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_ivec4(const std::string_view label, glm::ivec4& value, const glm::ivec4& min, const glm::ivec4& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<4, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec4(const glm::ivec4& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec4##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<4, int32_t>(label.data(), value, size, glm::zero<glm::ivec4>(), glm::zero<glm::ivec4>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec2(const std::string_view label, glm::vec2& value, const glm::vec2& min, const glm::vec2& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<2, float>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_vec2(const glm::vec2& value) {
      static size_t counter = 0;
      std::string label = std::format("vec2##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<2, float>(label.data(), value, size, glm::zero<glm::vec2>(), glm::zero<glm::vec2>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec3(const std::string_view label, glm::vec3& value, const glm::vec3& min, const glm::vec3& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<3, float>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_vec3(const glm::vec3& value) {
      static size_t counter = 0;
      std::string label = std::format("vec3##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<3, float>(label.data(), value, size, glm::zero<glm::vec3>(), glm::zero<glm::vec3>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec4(const std::string_view label, glm::vec4& value, const glm::vec4& min, const glm::vec4& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<4, float>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_vec4(const glm::vec4& value) {
      static size_t counter = 0;
      std::string label = std::format("vec4##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::edit_vecn<4, float>(label.data(), value, size, glm::zero<glm::vec4>(), glm::zero<glm::vec4>(), 1.f, ImGuiSliderFlags_None);
    }

  }  // namespace ui
}  // namespace other