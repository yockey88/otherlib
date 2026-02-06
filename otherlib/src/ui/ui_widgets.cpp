/**
 * \file ui/ui_widgets.cpp
 **/
#include "ui/ui_widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "renderer/ui/ui_helpers.hpp"

#include "colors.hpp"

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
          draw_list->AddRect(rect.Min, rect.Max, colors::rgba_to_hex(colors::kHighlight), rounding, 0, 1.5f);
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

    bool edit_ivec2(const std::string_view label, glm::ivec2& value, const glm::ivec2& min, const glm::ivec2& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<2, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec2(const glm::ivec2& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec2##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<2, int32_t>(label.data(), value, size, glm::zero<glm::ivec2>(), glm::zero<glm::ivec2>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_ivec3(const std::string_view label, glm::ivec3& value, const glm::ivec3& min, const glm::ivec3& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<3, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec3(const glm::ivec3& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec3##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<3, int32_t>(label.data(), value, size, glm::zero<glm::ivec3>(), glm::zero<glm::ivec3>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_ivec4(const std::string_view label, glm::ivec4& value, const glm::ivec4& min, const glm::ivec4& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<4, int32_t>(label.data(), value, size, min, max, speed, flags);
    }

    void draw_ivec4(const glm::ivec4& value) {
      static size_t counter = 0;
      std::string label = std::format("ivec4##{}", counter++);
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<4, int32_t>(label.data(), value, size, glm::zero<glm::ivec4>(), glm::zero<glm::ivec4>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec2(const std::string_view label, glm::vec2& value, const glm::vec2& min, const glm::vec2& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<2, float>(label, value, size, min, max, speed, flags);
    }

    void draw_vec2(const std::string_view label, const glm::vec2& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<2, float>(label, value, size, glm::zero<glm::vec2>(), glm::zero<glm::vec2>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec3(const std::string_view label, glm::vec3& value, const glm::vec3& min, const glm::vec3& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<3, float>(label, value, size, min, max, speed, flags);
    }

    void draw_vec3(const std::string_view label, const glm::vec3& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<3, float>(label, value, size, glm::zero<glm::vec3>(), glm::zero<glm::vec3>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_vec4(const std::string_view label, glm::vec4& value, const glm::vec4& min, const glm::vec4& max, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_vecn<4, float>(label, value, size, min, max, speed, flags);
    }

    void draw_vec4(const std::string_view label, const glm::vec4& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_vecn<4, float>(label, value, size, glm::zero<glm::vec4>(), glm::zero<glm::vec4>(), 1.f, ImGuiSliderFlags_None);
    }

    bool edit_quat(const std::string_view label, glm::quat& value, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      return detail::edit_quat(label, value, size, vmin, vmax, speed, flags);
    }

    void draw_quat(const std::string_view label, const glm::quat& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      detail::draw_quat(label, value, size, glm::quat{ 0.f, 0.f, 0.f, 0.f }, glm::quat{ 0.f, 0.f, 0.f, 0.f }, 1.f, ImGuiSliderFlags_None);
    }

    bool edit_mat3(const std::string_view label, glm::mat3& value, const glm::mat3& vmin, const glm::mat3& vmax, float speed, ImGuiSliderFlags flags) {
      constexpr float spacing = 8.f;

      scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
      scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

      shift_cursor_y(kFramePadding);

      const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
      const ImVec2 button_size = {
        line_height + 2.f,
        line_height
      };

      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      glm::vec3 i = value[0];
      glm::vec3 j = value[1];
      glm::vec3 k = value[2];

      bool changed = false;

      // Pre-format IDs to avoid dangling string_view
      std::string id_x0 = std::format("{}##{}:x0", label, label);
      std::string id_x1 = std::format("{}##{}:x1", label, label);
      std::string id_x2 = std::format("{}##{}:x2", label, label);
      std::string id_y0 = std::format("{}##{}:y0", label, label);
      std::string id_y1 = std::format("{}##{}:y1", label, label);
      std::string id_y2 = std::format("{}##{}:y2", label, label);
      std::string id_z0 = std::format("{}##{}:z0", label, label);
      std::string id_z1 = std::format("{}##{}:z1", label, label);
      std::string id_z2 = std::format("{}##{}:z2", label, label);

      /// all i.x, j.x, k.x on same line
      changed |= detail::draw_vector_element<float>(id_x0, "X0", i.x, size, vmin[0].x, vmax[0].x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_x1, "X1", j.x, size, vmin[1].x, vmax[1].x, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_x2, "X2", k.x, size, vmin[2].x, vmax[2].x, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, button_size, speed, flags);

      /// all i.y, j.y, k.y on same line
      // ImGui::NewLine();
      changed |= detail::draw_vector_element<float>(id_y0, "Y0", i.y, size, vmin[0].y, vmax[0].y, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_y1, "Y1", j.y, size, vmin[1].y, vmax[1].y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_y2, "Y2", k.y, size, vmin[2].y, vmax[2].y, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, button_size, speed, flags);

      /// all i.z, j.z, k.z on same line
      changed |= detail::draw_vector_element<float>(id_z0, "Z0", i.z, size, vmin[0].z, vmax[0].z, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_z1, "Z1", j.z, size, vmin[1].z, vmax[1].z, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, button_size, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_z2, "Z2", k.z, size, vmin[2].z, vmax[2].z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, button_size, speed, flags);

      if (changed) {
        value[0] = i;
        value[1] = j;
        value[2] = k;
      }

      return changed;
    }

    void draw_mat3(const std::string_view label, const glm::mat3& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      glm::mat3 value_copy = value;
      edit_mat3(label, value_copy);
    }

    bool edit_mat4(const std::string_view label, glm::mat4& value, const glm::mat4& vmin, const glm::mat4& vmax, float speed, ImGuiSliderFlags flags) {
      constexpr float spacing = 8.f;

      scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
      scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

      shift_cursor_y(kFramePadding);

      const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
      const ImVec2 button_size = {
        line_height + 2.f,
        line_height
      };

      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      glm::vec4 i = value[0];
      glm::vec4 j = value[1];
      glm::vec4 k = value[2];
      glm::vec4 l = value[3];
      bool changed = false;

      // Pre-format IDs to avoid dangling string_view
      std::string id_x0 = std::format("{}##{}:x0", label, label);
      std::string id_x1 = std::format("{}##{}:x1", label, label);
      std::string id_x2 = std::format("{}##{}:x2", label, label);
      std::string id_y0 = std::format("{}##{}:y0", label, label);
      std::string id_y1 = std::format("{}##{}:y1", label, label);
      std::string id_y2 = std::format("{}##{}:y2", label, label);
      std::string id_z0 = std::format("{}##{}:z0", label, label);
      std::string id_z1 = std::format("{}##{}:z1", label, label);
      std::string id_z2 = std::format("{}##{}:z2", label, label);
      std::string id_w0 = std::format("{}##{}:w0", label, label);
      std::string id_w1 = std::format("{}##{}:w1", label, label);
      std::string id_w2 = std::format("{}##{}:w2", label, label);

      /// all i.x, j.x, k.x on same line
      changed |= detail::draw_vector_element<float>(id_x0, "X0", i.x, size, vmin[0].x, vmax[0].x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_x1, "X1", j.x, size, vmin[1].x, vmax[1].x, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_x2, "X2", k.x, size, vmin[2].x, vmax[2].x, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);

      /// all i.y, j.y, k.y on same line
      // ImGui::NewLine();
      changed |= detail::draw_vector_element<float>(id_y0, "Y0", i.y, size, vmin[0].y, vmax[0].y, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_y1, "Y1", j.y, size, vmin[1].y, vmax[1].y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_y2, "Y2", k.y, size, vmin[2].y, vmax[2].y, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);

      /// all i.z, j.z, k.z on same line
      changed |= detail::draw_vector_element<float>(id_z0, "Z0", i.z, size, vmin[0].z, vmax[0].z, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_z1, "Z1", j.z, size, vmin[1].z, vmax[1].z, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_z2, "Z2", k.z, size, vmin[2].z, vmax[2].z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);

      /// all i.w, j.w, k.w on same line
      changed |= detail::draw_vector_element<float>(id_w0, "W0", i.w, size, vmin[0].w, vmax[0].w, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_w1, "W1", j.w, size, vmin[1].w, vmax[1].w, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);
      ImGui::SameLine();
      changed |= detail::draw_vector_element<float>(id_w2, "W2", k.w, size, vmin[2].w, vmax[2].w, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec2{ size.x / 3.f, size.y }, speed, flags);

      if (changed) {
        value[0] = i;
        value[1] = j;
        value[2] = k;
        value[3] = l;
      }
      return changed;
    }

    void draw_mat4(const std::string_view label, const glm::mat4& value) {
      ImVec2 size = ImVec2{ ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeight() * kLineHeightFactor };
      glm::mat4 value_copy = value;
      edit_mat4(label, value_copy);
    }

    namespace detail {

      bool draw_quat_elements(const std::string_view parent_label, const std::string_view label, glm::quat& value, const ImVec2& size, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags) {
        bool changed = false;

        std::string elem_id = std::format("##{}:{}", parent_label, label);
        glm::vec3 euler = glm::eulerAngles(value) * (180.f / glm::pi<float>());

        const float line_height = GImGui->Font->FontSize + kFramePadding * 2.f;
        /// use longest word "Pitch" to calculate button size so all 3 elements have same size
        const ImVec2 button_size = {
          ImGui::CalcTextSize("Pitch").x + line_height,
          line_height
        };

        changed |= detail::draw_vector_element<float>(elem_id, "Yaw", euler.y, size, vmin.y, vmax.y, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, ImVec4{ 0.3f, 0.8f, 0.3f, 1.f }, ImVec4{ 0.2f, 0.7f, 0.2f, 1.f }, button_size, speed, flags);
        changed |= detail::draw_vector_element<float>(elem_id, "Pitch", euler.x, size, vmin.x, vmax.x, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, ImVec4{ 0.9f, 0.2f, 0.2f, 1.f }, ImVec4{ 0.8f, 0.1f, 0.15f, 1.f }, button_size, speed, flags);
        changed |= detail::draw_vector_element<float>(elem_id, "Roll", euler.z, size, vmin.z, vmax.z, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, ImVec4{ 0.2f, 0.35f, 0.9f, 1.f }, ImVec4{ 0.1f, 0.25f, 0.8f, 1.f }, button_size, speed, flags);
        if (changed) {
          value = glm::quat(glm::radians(euler));
        }

        return changed;
      }

      bool edit_quat(const std::string_view label, glm::quat& value, ImVec2 size, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags) {
        const float spacing = 8.f;

        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        // ImGui::Text("%s", label.data());

        // /// shift down do y,j,g,etc.. doesn't get cut off by first element
        // shift_cursor_y(kFramePadding + 11.f);
        // underline();

        return detail::draw_quat_elements(label, label, value, size, vmin, vmax, speed, flags);
      }

      void draw_quat(const std::string_view label, const glm::quat& value, ImVec2 size, const glm::quat& vmin, const glm::quat& vmax, float speed, ImGuiSliderFlags flags) {
        const float spacing = 8.f;

        scoped_style item_spacing(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        scoped_style padding(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        shift_cursor_y(kFramePadding);

        // ImGui::Text("%s", label.data());
        // underline();

        glm::quat temp = value;
        detail::draw_quat_elements(label, label, temp, size, vmin, vmax, speed, flags);
      }

    }  // namespace detail
  }  // namespace ui
}  // namespace other