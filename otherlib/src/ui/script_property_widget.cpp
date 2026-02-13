/**
 * \file ui/script_property_widget.cpp
 **/
#include "ui/script_property_widget.hpp"

#include <cstring>

#include "core/logger.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_helpers.hpp"

namespace other {
  namespace ui {
    namespace detail {

      constexpr size_t kFieldValueBufferSize = 1024;

      std::string format_field_value(value_type type, const uint8_t* data, int32_t size);

    }  // namespace detail

    static bool s_behavior_section_open = false;

    bool begin_behavior_section(const std::string_view behavior_name, const std::string_view full_type_name, int32_t behavior_index) {
      ImDrawList* dl = ImGui::GetWindowDrawList();
      const ImVec2 cursor = ImGui::GetCursorScreenPos();
      const float avail_w = ImGui::GetContentRegionAvail().x;

      constexpr float kBehaviorHeaderHeight = 24.f;
      constexpr float kBehaviorDotRadius = 2.5f;

      std::string section_id = std::format("##behavior_{}", behavior_index);
      ImGui::PushID(section_id.c_str());

      /// header background
      ImVec2 header_min = cursor;
      ImVec2 header_max = { cursor.x + avail_w, cursor.y + kBehaviorHeaderHeight };

      bool hovered = ImGui::IsMouseHoveringRect(header_min, header_max);
      ImU32 bg_color = colors::to_im_col(hovered ? glm::vec4(0.18f, 0.18f, 0.20f, 1.0f) : glm::vec4(0.14f, 0.14f, 0.16f, 1.0f));
      dl->AddRectFilled(header_min, header_max, bg_color);

      /// script signature dot (using script component color)
      float dot_cx = cursor.x + inspector::kInnerPadding + 4.f;
      float dot_cy = cursor.y + kBehaviorHeaderHeight * 0.5f;
      dl->AddCircleFilled(
        { dot_cx, dot_cy },
        kBehaviorDotRadius,
        colors::to_im_col(colors::scene_object::kComponentScript)
      );

      /// behavior display name
      float text_x = dot_cx + kBehaviorDotRadius + 8.f;
      float text_y = cursor.y + (kBehaviorHeaderHeight - ImGui::GetFontSize()) * 0.5f;
      dl->AddText({ text_x, text_y }, colors::to_im_col(glm::vec4(0.85f, 0.87f, 0.90f, 1.0f)), behavior_name.data());

      /// full type name (right-aligned, muted)
      {
        std::string ns = std::string(full_type_name);
        size_t dot = ns.rfind('.');
        if (dot != std::string::npos) {
          ns = ns.substr(0, dot);
        }
        float ns_w = ImGui::CalcTextSize(ns.c_str()).x;
        float ns_x = cursor.x + avail_w - inspector::kInnerPadding - ns_w;
        dl->AddText({ ns_x, text_y }, colors::to_im_col(glm::vec4(0.45f, 0.45f, 0.50f, 1.0f)), ns.c_str());
      }

      /// click to toggle
      ImGui::SetCursorScreenPos(header_min);
      std::string btn_id = std::format("##beh_hdr_{}", behavior_index);
      if (ImGui::InvisibleButton(btn_id.c_str(), { avail_w, kBehaviorHeaderHeight })) {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        ImGuiID state_id = ImGui::GetID("##beh_open");
        bool was_open = storage->GetBool(state_id, true);
        storage->SetBool(state_id, !was_open);
      }

      /// check state
      {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        ImGuiID state_id = ImGui::GetID("##beh_open");
        s_behavior_section_open = storage->GetBool(state_id, true);
      }

      /// separator below header
      dl->AddLine(
        { cursor.x, header_max.y },
        { cursor.x + avail_w, header_max.y },
        colors::to_im_col(glm::vec4(0.22f, 0.22f, 0.24f, 1.0f))
      );

      ImGui::SetCursorScreenPos({ cursor.x, header_max.y + 1.f });

      if (s_behavior_section_open) {
        shift_cursor_y(2.f);
      }

      return s_behavior_section_open;
    }

    void end_behavior_section() {
      if (s_behavior_section_open) {
        shift_cursor_y(4.f);
      }
      ImGui::PopID();
    }

    bool begin_field_group(const std::string_view group_name) {
      std::string id = std::format("##group_{}", group_name);
      return ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", group_name.data());
    }

    void end_field_group(bool was_open) {
      if (was_open) {
        ImGui::TreePop();
      }
    }

    void draw_field_tooltip(const std::string_view tooltip) {
      if (tooltip.empty()) {
        return;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip.data(), tooltip.data() + tooltip.size());
        ImGui::EndTooltip();
      }
    }

    void draw_no_behaviors_message() {
      scoped_color text_col(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextDisabled));
      shift_cursor_y(4.f);
      ImGui::TextWrapped("No behaviors attached.");
      shift_cursor_y(4.f);
    }

    bool draw_behavior_field(integer_t script_object_id, const behavior_descriptor& behavior, const behavior_field_descriptor& field) {
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment is not available");

      auto* script_obj = env->get_object(script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found", script_object_id);

      /// determine which label to show
      const std::string& label = field.display_name.empty() ? field.field_name : field.display_name;
      bool is_read_only = has_flag(field.flags, behavior_display_flags::read_only);
      bool is_color = has_flag(field.flags, behavior_display_flags::color_field);
      bool has_range = has_flag(field.flags, behavior_display_flags::has_range);

      /// read the current value from the managed side
      uint8_t buffer[detail::kFieldValueBufferSize] = {};
      int32_t bytes_read = script_obj->dotnet_object->read_behavior_field_value(behavior.behavior_index, field.field_index, buffer, detail::kFieldValueBufferSize);

      if (bytes_read <= 0 && field.type != value_type::OEBOOL) {
        inspector::property_display(label, "<unreadable>", colors::kTextDisabled);
        return false;
      }

      bool changed = false;

      if (is_read_only) {
        std::string val_str = detail::format_field_value(field.type, buffer, bytes_read);
        inspector::property_display(label, val_str, colors::kTextMuted);
        if (has_flag(field.flags, behavior_display_flags::has_tooltip)) {
          draw_field_tooltip(field.tooltip);
        }
        return false;
      }

      switch (field.type) {
        case value_type::OEBOOL: {
          bool val = *reinterpret_cast<bool*>(buffer);
          if (inspector::property_bool(label, val)) {
            script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &val, sizeof(val));
            changed = true;
          }
        } break;

        case value_type::INT8:
        case value_type::INT16:
        case value_type::INT32:
        case value_type::INT64: {
          int64_t val = 0;
          switch (field.type) {
            case value_type::INT8: val = *reinterpret_cast<int8_t*>(buffer); break;
            case value_type::INT16: val = *reinterpret_cast<int16_t*>(buffer); break;
            case value_type::INT32: val = *reinterpret_cast<int32_t*>(buffer); break;
            case value_type::INT64: val = *reinterpret_cast<int64_t*>(buffer); break;
            default: break;
          }

          bool field_changed = false;
          if (inspector::property_int64(label, val)) {
            field_changed = true;
          }

          if (field_changed) {
            switch (field.type) {
              case value_type::INT8: {
                int8_t v = static_cast<int8_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::INT16: {
                int16_t v = static_cast<int16_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::INT32: {
                int32_t v = static_cast<int32_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::INT64: {
                int64_t v = static_cast<int64_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              default: break;
            }
            changed = true;
          }
        } break;

        case value_type::UINT8:
        case value_type::UINT16:
        case value_type::UINT32:
        case value_type::UINT64: {
          uint64_t val = 0;
          switch (field.type) {
            case value_type::UINT8: val = *reinterpret_cast<uint8_t*>(buffer); break;
            case value_type::UINT16: val = *reinterpret_cast<uint16_t*>(buffer); break;
            case value_type::UINT32: val = *reinterpret_cast<uint32_t*>(buffer); break;
            case value_type::UINT64: val = *reinterpret_cast<uint64_t*>(buffer); break;
            default: break;
          }

          if (inspector::property_uint64(label, val)) {
            switch (field.type) {
              case value_type::UINT8: {
                uint8_t v = static_cast<uint8_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::UINT16: {
                uint16_t v = static_cast<uint16_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::UINT32: {
                uint32_t v = static_cast<uint32_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              case value_type::UINT64: {
                uint64_t v = static_cast<uint64_t>(val);
                script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &v, sizeof(v));
              } break;
              default: break;
            }
            changed = true;
          }
        } break;

        case value_type::FLOAT: {
          float val = *reinterpret_cast<float*>(buffer);

          bool field_changed = false;
          if (is_color) {
            /// treat as single channel — unusual but handle it
            inspector::begin_property_row(label);
            std::string id = std::format("##{}", label);
            field_changed = ImGui::ColorEdit3(id.c_str(), &val, ImGuiColorEditFlags_Float);
            inspector::end_property_row();
          } else if (has_range) {
            inspector::begin_property_row(label);
            std::string id = std::format("##{}", label);
            field_changed = ImGui::SliderFloat(id.c_str(), &val, field.range_min, field.range_max);
            inspector::end_property_row();
          } else {
            field_changed = inspector::property_float(label, val);
          }

          if (field_changed) {
            script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &val, sizeof(val));
            changed = true;
          }
        } break;

        case value_type::DOUBLE: {
          double val = *reinterpret_cast<double*>(buffer);
          float fval = static_cast<float>(val);
          bool field_changed = false;

          if (has_range) {
            inspector::begin_property_row(label);
            std::string id = std::format("##{}", label);
            field_changed = ImGui::SliderFloat(id.c_str(), &fval, field.range_min, field.range_max);
            inspector::end_property_row();
          } else {
            field_changed = inspector::property_float(label, fval);
          }

          if (field_changed) {
            val = static_cast<double>(fval);
            script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &val, sizeof(val));
            changed = true;
          }
        } break;

        case value_type::STRING: {
          char text_buf[256] = {};
          size_t copy_len = std::min(static_cast<size_t>(bytes_read), sizeof(text_buf) - 1);
          std::memcpy(text_buf, buffer, copy_len);
          text_buf[copy_len] = '\0';

          if (inspector::property_text(label, text_buf, sizeof(text_buf))) {
            size_t new_len = std::strlen(text_buf) + 1;
            script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, text_buf, static_cast<int32_t>(new_len));
            changed = true;
          }
        } break;

        case value_type::CHAR: {
          char val = *reinterpret_cast<char*>(buffer);
          char text_buf[4] = { val, '\0' };
          if (inspector::property_text(label, text_buf, sizeof(text_buf))) {
            val = text_buf[0];
            script_obj->dotnet_object->write_field_value(behavior.behavior_index, field.field_index, &val, sizeof(val));
            changed = true;
          }
        } break;

        default: {
          std::string type_str = get_value_type_string_from_type(field.type);
          inspector::property_display(label, std::format("<{}>", type_str), colors::kTextDisabled);
        } break;
      }

      /// tooltip on hover
      if (has_flag(field.flags, behavior_display_flags::has_tooltip)) {
        draw_field_tooltip(field.tooltip);
      }

      return changed;
    }

    namespace detail {

      std::string format_field_value(value_type type, const uint8_t* data, int32_t size) {
        if (data == nullptr || size <= 0) {
          return "<null>";
        }

        switch (type) {
          case value_type::OEBOOL: return *reinterpret_cast<const bool*>(data) ? "true" : "false";
          case value_type::CHAR: return std::format("'{}'", *reinterpret_cast<const char*>(data));
          case value_type::INT8: return std::format("{}", *reinterpret_cast<const int8_t*>(data));
          case value_type::INT16: return std::format("{}", *reinterpret_cast<const int16_t*>(data));
          case value_type::INT32: return std::format("{}", *reinterpret_cast<const int32_t*>(data));
          case value_type::INT64: return std::format("{}", *reinterpret_cast<const int64_t*>(data));
          case value_type::UINT8: return std::format("{}", *reinterpret_cast<const uint8_t*>(data));
          case value_type::UINT16: return std::format("{}", *reinterpret_cast<const uint16_t*>(data));
          case value_type::UINT32: return std::format("{}", *reinterpret_cast<const uint32_t*>(data));
          case value_type::UINT64: return std::format("{}", *reinterpret_cast<const uint64_t*>(data));
          case value_type::FLOAT: return std::format("{:.3f}", *reinterpret_cast<const float*>(data));
          case value_type::DOUBLE: return std::format("{:.3f}", *reinterpret_cast<const double*>(data));
          case value_type::STRING: return std::string(reinterpret_cast<const char*>(data), size - 1);
          default: return "<unsupported>";
        }
      }

    }  // namespace detail
  }  // namespace ui
}  // namespace other