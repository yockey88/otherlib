/**
 * \file ui/inspector_widgets.cpp
 **/
#include "ui/inspector_widgets.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_helpers.hpp"
#include "renderer/ui/unicode.hpp"


namespace other {
  namespace ui {
    namespace inspector {

      static void draw_filled_circle(ImDrawList* dl, const ImVec2& center, float radius, ImU32 color) {
        dl->AddCircleFilled(center, radius, color);
      }

      glm::vec4 get_component_color(component::id tag) {
        switch (tag) {
          case component::id::TRANSFORM: return colors::scene_object::kComponentTransform;
          case component::id::RENDERER: return colors::scene_object::kComponentRenderer;
          case component::id::PHYSICS: return colors::scene_object::kComponentPhysics;
          case component::id::SCRIPT: return colors::scene_object::kComponentScript;
          case component::id::AUDIO: return colors::scene_object::kComponentAudio;
          case component::id::LIGHT: return colors::scene_object::kComponentLight;
          case component::id::CAMERA: return colors::scene_object::kComponentCamera;
          default: return colors::scene_object::kComponentCustom;
        }
      }

      void draw_object_header(const std::string_view object_name, natural_t object_id, const glm::vec4& icon_color) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// background
        ImVec2 header_min = cursor;
        ImVec2 header_max = { cursor.x + avail_w, cursor.y + kObjectHeaderHeight };
        dl->AddRectFilled(header_min, header_max, colors::to_im_col(colors::inspector::kObjectHeaderBG), 0.f);

        /// icon square
        /// \todo replace with texture icon?
        float icon_x = cursor.x + kInnerPadding;
        float icon_y = cursor.y + (kObjectHeaderHeight - kIconSize) * 0.5f;
        dl->AddRectFilled(
          { icon_x, icon_y },
          { icon_x + kIconSize, icon_y + kIconSize },
          colors::to_im_col(icon_color),
          4.f
        );

        /// object name
        float text_x = icon_x + kIconSize + 8.f;
        float text_y = cursor.y + (kObjectHeaderHeight - ImGui::GetFontSize()) * 0.5f;
        dl->AddText({ text_x, text_y }, colors::to_im_col(colors::inspector::kObjectName), object_name.data());

        /// object id (right-aligned)
        std::string id_str = std::format("#0x{:04X}", object_id);
        float id_w = ImGui::CalcTextSize(id_str.c_str()).x;
        float id_x = cursor.x + avail_w - kInnerPadding - id_w;
        dl->AddText({ id_x, text_y }, colors::to_im_col(colors::inspector::kObjectID), id_str.c_str());

        /// separator line
        dl->AddLine(
          { cursor.x, header_max.y },
          { cursor.x + avail_w, header_max.y },
          colors::to_im_col(colors::inspector::kBorder)
        );

        ImGui::Dummy({ avail_w, kObjectHeaderHeight + 1.f });
      }

      bool draw_object_header_editable(char* name_buf, uint32_t buf_size, natural_t object_id, const glm::vec4& icon_color) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// background
        ImVec2 header_min = cursor;
        ImVec2 header_max = { cursor.x + avail_w, cursor.y + kObjectHeaderHeight };
        dl->AddRectFilled(header_min, header_max, colors::to_im_col(colors::inspector::kObjectHeaderBG), 0.f);

        /// icon square
        float icon_x = cursor.x + kInnerPadding;
        float icon_y = cursor.y + (kObjectHeaderHeight - kIconSize) * 0.5f;
        dl->AddRectFilled(
          { icon_x, icon_y },
          { icon_x + kIconSize, icon_y + kIconSize },
          colors::to_im_col(icon_color),
          4.f
        );

        /// object id label (right side, drawn first so we know remaining width)
        std::string id_str = std::format("#0x{:04X}", object_id);
        float id_w = ImGui::CalcTextSize(id_str.c_str()).x;
        float id_x = cursor.x + avail_w - kInnerPadding - id_w;
        float text_y = cursor.y + (kObjectHeaderHeight - ImGui::GetFontSize()) * 0.5f;
        dl->AddText({ id_x, text_y }, colors::to_im_col(colors::inspector::kObjectID), id_str.c_str());

        /// editable name field
        float name_x = icon_x + kIconSize + 8.f;
        float name_w = id_x - name_x - 8.f;
        float name_y = cursor.y + (kObjectHeaderHeight - ImGui::GetFrameHeight()) * 0.5f;

        ImGui::SetCursorScreenPos({ name_x, name_y });
        ImGui::PushItemWidth(name_w);
        {
          scoped_color bg(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::inspector::kObjectHeaderBG));
          scoped_color text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kObjectName));
          scoped_color border(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
          scoped_style frame_padding(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f));

          bool changed = ImGui::InputText("##obj-name", name_buf, buf_size, ImGuiInputTextFlags_EnterReturnsTrue);
          ImGui::PopItemWidth();

          /// separator line
          dl->AddLine(
            { cursor.x, header_max.y },
            { cursor.x + avail_w, header_max.y },
            colors::to_im_col(colors::inspector::kBorder)
          );

          /// advance cursor past the header
          ImGui::SetCursorScreenPos({ cursor.x, header_max.y + 1.f });

          return changed;
        }
      }

      /// internal stack to track open/closed state per section
      static bool s_component_section_open = false;

      bool begin_component_section(const std::string_view component_name, component::id tag, component_section_flags flags, bool* out_remove_requested) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;
        const glm::vec4 dot_color = get_component_color(tag);

        ImGui::PushID(component_name.data());

        /// header background — use invisible button for click detection
        ImVec2 header_min = cursor;
        ImVec2 header_max = { cursor.x + avail_w, cursor.y + kComponentHeaderHeight };

        bool hovered = ImGui::IsMouseHoveringRect(header_min, header_max);
        ImU32 header_bg = colors::to_im_col(hovered ? colors::inspector::kComponentHeaderHover : colors::inspector::kComponentHeaderBG);
        dl->AddRectFilled(header_min, header_max, header_bg);

        /// component dot
        float dot_cx = cursor.x + kInnerPadding;
        float dot_cy = cursor.y + kComponentHeaderHeight * 0.5f;
        draw_filled_circle(dl, { dot_cx, dot_cy }, kComponentDotRadius, colors::to_im_col(dot_color));

        /// component name text
        float text_x = dot_cx + kComponentDotRadius + 8.f;
        float text_y = cursor.y + (kComponentHeaderHeight - ImGui::GetFontSize()) * 0.5f;
        dl->AddText({ text_x, text_y }, colors::to_im_col(colors::inspector::kComponentHeaderText), component_name.data());

        /// modified marker (right side)
        if (flags.modified) {
          std::string mod_text_str = std::string(unicode::kStatusDot) + " modified";
          const char* mod_text = mod_text_str.c_str();

          float mod_w = ImGui::CalcTextSize(mod_text).x;
          float mod_x = cursor.x + avail_w - kInnerPadding - mod_w;
          dl->AddText({ mod_x, text_y }, colors::to_im_col(colors::inspector::kModifiedMarker), mod_text);
        }

        /// remove button (right side, before modified marker)
        if (flags.removable && out_remove_requested) {
          float btn_x = cursor.x + avail_w - kInnerPadding - (flags.modified ? 80.f : 0.f) - 16.f;
          float btn_y = cursor.y + (kComponentHeaderHeight - 12.f) * 0.5f;
          ImGui::SetCursorScreenPos({ btn_x, btn_y });
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::rgba_to_imvec4(colors::inspector::kComponentHeaderHover));
          ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kResetButton));
          if (ImGui::SmallButton("x")) {
            *out_remove_requested = true;
          }
          ImGui::PopStyleColor(3);
        }

        /// click to toggle
        ImGui::SetCursorScreenPos(header_min);
        std::string btn_id = std::format("##comp_hdr_{}", component_name);
        if (ImGui::InvisibleButton(btn_id.c_str(), { avail_w, kComponentHeaderHeight })) {
          /// toggle stored in ImGui internal storage
          ImGuiStorage* storage = ImGui::GetStateStorage();
          ImGuiID state_id = ImGui::GetID("##comp_open");
          bool was_open = storage->GetBool(state_id, true);
          storage->SetBool(state_id, !was_open);
        }

        /// check open state
        {
          ImGuiStorage* storage = ImGui::GetStateStorage();
          ImGuiID state_id = ImGui::GetID("##comp_open");
          s_component_section_open = storage->GetBool(state_id, true);
        }

        /// separator below header
        dl->AddLine(
          { cursor.x, header_max.y },
          { cursor.x + avail_w, header_max.y },
          colors::to_im_col(colors::inspector::kComponentSeparator)
        );

        /// advance cursor past header
        ImGui::SetCursorScreenPos({ cursor.x, header_max.y + 1.f });

        if (s_component_section_open) {
          /// push body background color
          ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::inspector::kComponentBody));

          /// begin a region for the body contents
          /// using small vertical padding
          shift_cursor_y(2.f);
          // shift_cursor_x(kInnerPadding);
        }

        return s_component_section_open;
      }

      void end_component_section() {
        if (s_component_section_open) {
          shift_cursor_y(4.f);
          ImGui::PopStyleColor();  /// kComponentBody
        }
        ImGui::PopID();
      }

      void begin_property_row(const std::string_view label, float label_width) {
        ImGui::PushID(label.data());

        /// property row is a horizontal group: label on left, fields on right
        const float avail_w = ImGui::GetContentRegionAvail().x;

        /// clamp label width: at least kPropertyLabelMinWidth, at most 55% of available
        float effective_label_w = label_width;
        float max_label_w = avail_w * 0.55f;
        if (effective_label_w > max_label_w) {
          effective_label_w = max_label_w;
        }
        if (effective_label_w < kPropertyLabelMinWidth) {
          effective_label_w = kPropertyLabelMinWidth;
        }

        float field_region_w = avail_w - effective_label_w - kInnerPadding;
        if (field_region_w > kFieldMaxWidth) {
          field_region_w = kFieldMaxWidth;
        }

        /// label — clipped to the label column so long names don't bleed into fields
        {
          scoped_color text_col(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kPropertyLabel));
          ImGui::AlignTextToFramePadding();

          const ImVec2 cursor = ImGui::GetCursorScreenPos();
          const float clip_right = cursor.x + effective_label_w - kFieldGap;
          ImGui::PushClipRect(
            cursor,
            ImVec2(clip_right, cursor.y + ImGui::GetFrameHeight()),
            true
          );
          ImGui::Text("%s", label.data());
          ImGui::PopClipRect();
        }

        /// same line, then offset to field column
        ImGui::SameLine(effective_label_w);
        ImGui::PushItemWidth(field_region_w);
      }

      void end_property_row() {
        ImGui::PopItemWidth();
        ImGui::PopID();
      }

      static void push_field_style(const glm::vec4* axis_color = nullptr) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::inspector::kFieldBorder));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kPropertyValueText));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, (kFieldHeight - ImGui::GetFontSize()) * 0.5f));
      }

      static void pop_field_style() {
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
      }

      /// draw colored left-border accent on the last drawn item
      static void draw_axis_accent(const glm::vec4& axis_color) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 item_min = ImGui::GetItemRectMin();
        ImVec2 item_max = ImGui::GetItemRectMax();
        dl->AddRectFilled(
          { item_min.x, item_min.y },
          { item_min.x + 2.f, item_max.y },
          colors::to_im_col(axis_color),
          3.f,
          ImDrawFlags_RoundCornersLeft
        );
      }

      /// draw focused border accent on the last drawn item if active
      static void draw_focus_border_if_active() {
        if (ImGui::IsItemActive()) {
          ImDrawList* dl = ImGui::GetWindowDrawList();
          ImVec2 item_min = ImGui::GetItemRectMin();
          ImVec2 item_max = ImGui::GetItemRectMax();
          dl->AddRect(item_min, item_max, colors::to_im_col(colors::inspector::kFieldBorderFocused), 3.f);
        }
      }

      bool input_float_field(const char* id, float& value, float speed, const glm::vec4* axis_color) {
        push_field_style(axis_color);
        bool changed = ImGui::DragFloat(id, &value, speed, 0.f, 0.f, "%.2f");
        if (axis_color) {
          draw_axis_accent(*axis_color);
        }
        draw_focus_border_if_active();
        pop_field_style();
        return changed;
      }

      bool input_int_field(const char* id, int32_t& value, const glm::vec4* axis_color) {
        push_field_style(axis_color);
        bool changed = ImGui::DragInt(id, &value, 1.f, 0, 0);
        if (axis_color) {
          draw_axis_accent(*axis_color);
        }
        draw_focus_border_if_active();
        pop_field_style();
        return changed;
      }

      bool input_text_field(const char* id, char* buf, uint32_t buf_size) {
        push_field_style();
        bool changed = ImGui::InputText(id, buf, buf_size);
        draw_focus_border_if_active();
        pop_field_style();
        return changed;
      }

      void display_text_field(const char* id, const std::string_view text) {
        push_field_style();
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kPropertyValueText));
        ImGui::BeginDisabled();
        /// use a disabled input text as a read-only display
        char buf[256];
        std::strncpy(buf, text.data(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        ImGui::InputText(id, buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        pop_field_style();
      }

      bool inspector_checkbox(const char* id, bool& value) {
        ImGui::PushStyleColor(ImGuiCol_CheckMark, colors::rgba_to_imvec4(colors::kCheckMark));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_Border, colors::rgba_to_imvec4(colors::inspector::kFieldBorder));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

        bool changed = ImGui::Checkbox(id, &value);

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
        return changed;
      }

      namespace detail {

        template <size_t N, typename T>
        bool property_vecn(const std::string_view label, glm::vec<N, T>& value, float speed) {
          bool changed = false;
          /// vec4 rendered as 4 fields (xyz + w)
          inspector::begin_property_row(label);
          const float field_w = (ImGui::GetContentRegionAvail().x - inspector::kFieldGap * 3.f) / 4.f;
          ImGui::PushItemWidth(field_w);
          {
            std::string xid = std::format("##{}_x", label);
            changed |= inspector::input_float_field(xid.c_str(), value.x, 0.1f, &colors::inspector::kVecFieldX);
          }
          ImGui::SameLine(0.f, inspector::kFieldGap);
          {
            std::string yid = std::format("##{}_y", label);
            changed |= inspector::input_float_field(yid.c_str(), value.y, 0.1f, &colors::inspector::kVecFieldY);
          }
          if constexpr (N > 2) {
            ImGui::SameLine(0.f, inspector::kFieldGap);
            {
              std::string zid = std::format("##{}_z", label);
              changed |= inspector::input_float_field(zid.c_str(), value.z, 0.1f, &colors::inspector::kVecFieldZ);
            }
          }
          if constexpr (N > 3) {
            ImGui::SameLine(0.f, inspector::kFieldGap);
            {
              std::string wid = std::format("##{}_w", label);
              changed |= inspector::input_float_field(wid.c_str(), value.w, 0.1f, &colors::inspector::kVecFieldW);
            }
          }
          ImGui::PopItemWidth();
          inspector::end_property_row();
          return changed;
        }

        template <size_t N, typename T>
        void property_vecn_readonly(const std::string_view label, const glm::vec<N, T>& value, float speed) {
          begin_property_row(label);

          const float field_w = (ImGui::GetContentRegionAvail().x - kFieldGap * 2.f) / 3.f;
          ImGui::PushItemWidth(field_w);

          push_field_style(&colors::inspector::kVecFieldX);
          ImGui::BeginDisabled();
          float x = value.x;
          ImGui::DragFloat(std::format("##{}:x", label).c_str(), &x, 0.f, 0.f, 0.f, "%.2f");
          draw_axis_accent(colors::inspector::kVecFieldX);
          ImGui::EndDisabled();
          pop_field_style();

          ImGui::SameLine(0.f, kFieldGap);
          push_field_style(&colors::inspector::kVecFieldY);
          ImGui::BeginDisabled();
          float y = value.y;
          ImGui::DragFloat(std::format("##{}:y", label).c_str(), &y, 0.f, 0.f, 0.f, "%.2f");
          draw_axis_accent(colors::inspector::kVecFieldY);
          ImGui::EndDisabled();
          pop_field_style();

          if constexpr (N > 2) {
            ImGui::SameLine(0.f, kFieldGap);

            push_field_style(&colors::inspector::kVecFieldZ);
            ImGui::BeginDisabled();
            float z = value.z;
            ImGui::DragFloat(std::format("##{}:z", label).c_str(), &z, 0.f, 0.f, 0.f, "%.2f");
            draw_axis_accent(colors::inspector::kVecFieldZ);
            ImGui::EndDisabled();
            pop_field_style();
          }

          if constexpr (N > 3) {
            ImGui::SameLine(0.f, kFieldGap);

            push_field_style(&colors::inspector::kVecFieldW);
            ImGui::BeginDisabled();
            float w = value.w;
            ImGui::DragFloat(std::format("##{}:w", label).c_str(), &w, 0.f, 0.f, 0.f, "%.2f");
            draw_axis_accent(colors::inspector::kVecFieldW);
            ImGui::EndDisabled();
            pop_field_style();
          }

          ImGui::PopItemWidth();
          end_property_row();
        }

      }  // namespace detail

      bool property_mat4(const std::string_view label, glm::mat4& value, float speed) {
        glm::vec4 row0 = value[0];
        glm::vec4 row1 = value[1];
        glm::vec4 row2 = value[2];
        glm::vec4 row3 = value[3];
        bool changed = false;
        changed |= detail::property_vecn<4>("X0", row0, speed);
        changed |= detail::property_vecn<4>("X1", row1, speed);
        changed |= detail::property_vecn<4>("X2", row2, speed);
        changed |= detail::property_vecn<4>("X3", row3, speed);
        if (changed) {
          value[0] = row0;
          value[1] = row1;
          value[2] = row2;
          value[3] = row3;
        }
        return changed;
      }

      void property_mat4_readonly(const std::string_view label, const glm::mat4& value) {
        glm::vec4 row0 = value[0];
        glm::vec4 row1 = value[1];
        glm::vec4 row2 = value[2];
        glm::vec4 row3 = value[3];
        detail::property_vecn_readonly<4>("X0", row0, 0.f);
        detail::property_vecn_readonly<4>("X1", row1, 0.f);
        detail::property_vecn_readonly<4>("X2", row2, 0.f);
        detail::property_vecn_readonly<4>("X3", row3, 0.f);
      }

      bool property_mat3(const std::string_view label, glm::mat3& value, float speed) {
        glm::vec3 row0 = value[0];
        glm::vec3 row1 = value[1];
        glm::vec3 row2 = value[2];
        bool changed = false;
        changed |= detail::property_vecn<3>("X0", row0, speed);
        changed |= detail::property_vecn<3>("X1", row1, speed);
        changed |= detail::property_vecn<3>("X2", row2, speed);
        if (changed) {
          value[0] = row0;
          value[1] = row1;
          value[2] = row2;
        }
        return changed;
      }

      void property_mat3_readonly(const std::string_view label, const glm::mat3& value) {
        glm::vec3 row0 = value[0];
        glm::vec3 row1 = value[1];
        glm::vec3 row2 = value[2];
        detail::property_vecn_readonly<3>("X0", row0, 0.f);
        detail::property_vecn_readonly<3>("X1", row1, 0.f);
        detail::property_vecn_readonly<3>("X2", row2, 0.f);
      }

      bool property_vec4(const std::string_view label, glm::vec4& value, float speed) {
        return detail::property_vecn<4>(label, value, speed);
      }

      void property_vec4_readonly(const std::string_view label, const glm::vec4& value) {
        detail::property_vecn_readonly<4>(label, value, 0.1f);
      }

      bool property_vec3(const std::string_view label, glm::vec3& value, float speed) {
        return detail::property_vecn<3>(label, value, speed);
      }

      void property_vec3_readonly(const std::string_view label, const glm::vec3& value) {
        detail::property_vecn_readonly<3>(label, value, 0.1f);
      }

      bool property_vec2(const std::string_view label, glm::vec2& value, float speed) {
        return detail::property_vecn<2>(label, value, speed);
      }

      void property_vec2_readonly(const std::string_view label, const glm::vec2& value) {
        detail::property_vecn_readonly<2>(label, value, 0.1f);
      }

      bool property_float(const std::string_view label, float& value, float speed) {
        begin_property_row(label);
        std::string id = std::format("##{}", label);
        bool changed = input_float_field(id.c_str(), value, speed);
        end_property_row();
        return changed;
      }

      bool property_int(const std::string_view label, int32_t& value) {
        begin_property_row(label);
        std::string id = std::format("##{}", label);
        bool changed = input_int_field(id.c_str(), value);
        end_property_row();
        return changed;
      }

      bool property_bool(const std::string_view label, bool& value) {
        begin_property_row(label);
        std::string id = std::format("##{}", label);
        bool changed = inspector_checkbox(id.c_str(), value);
        end_property_row();
        return changed;
      }

      bool property_text(const std::string_view label, char* buf, uint32_t buf_size) {
        begin_property_row(label);
        std::string id = std::format("##{}", label);
        bool changed = input_text_field(id.c_str(), buf, buf_size);
        end_property_row();
        return changed;
      }

      void property_display(const std::string_view label, const std::string_view value_text, const glm::vec4& text_color) {
        begin_property_row(label);
        /// if a custom color is provided (non-zero alpha), use it
        if (text_color.a > 0.f) {
          ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(text_color));
        }
        std::string id = std::format("##{}", label);
        display_text_field(id.c_str(), value_text);
        if (text_color.a > 0.f) {
          ImGui::PopStyleColor();
        }
        end_property_row();
      }

      void draw_asset_slot(const std::string_view label, const std::string_view asset_name, asset_slot_state state) {
      }

      bool draw_add_component_button() {
        return false;
      }

      void draw_no_selection_message() {
      }

      void draw_multi_selection_message(uint32_t count) {
      }

    }  // namespace inspector
  }  // namespace ui
}  // namespace other