/**
 * \file ui/type-bindings/value_ui.cpp
 **/
#include "ui/type-bindings/value_ui.hpp"

#include <imgui/imgui_memory_editor.h>

#include "core/profiler.hpp"
#include "theme/colors.hpp"
#include "ui/ui_helpers.hpp"
#include "ui/ui_widgets.hpp"

namespace other {
  namespace ui {

    void begin_property(const char* label, const char* help_text) {
      shift_cursor(10.f, 9.f);
      ImGui::Text("%s", label);

      if (std::strlen(help_text) != 0) {
        ImGui::SameLine();
        help_marker(help_text);
      }

      ImGui::NextColumn();
      shift_cursor_y(4.f);
      ImGui::PushItemWidth(-1);
    }

    void end_property() {
      ImGui::PopItemWidth();
      ImGui::NextColumn();
      underline();
    }

    void begin_property_grid(uint32_t columns, const ImVec2& item_spacing, const ImVec2& frame_pading) {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
      ImGui::Columns(columns);
    }

    void end_property_grid() {
      ImGui::Columns(1);
      underline();
      ImGui::PopStyleVar(2);  // ItemSpacing, FramePadding
      shift_cursor_y(18.0f);
    }

    std::string edit_string(const char* label, const std::string& val) {
      std::string edited_value = val;

      char buffer[1024];
      std::strncpy(buffer, val.c_str(), val.size() + 1);
      buffer[val.size()] = '\0';
      if (ImGui::InputText(label, buffer, 1024)) {
        // edited_value = std::string(buffer);
      }

      return edited_value;
    }

    bool edit_value(const char* label, value& val) {
      PROFILE_SECTION("edit_value");
      bool changed = false;

      switch (val.type()) {
        case value_type::STRING: {
          std::string str_val = val;

          char buffer[1024];
          std::strncpy(buffer, str_val.c_str(), val.size() + 1);
          buffer[val.size()] = '\0';

          shift_cursor_y(7.0f);

          std::string txt_label = std::format("{}##string-editor", label);
          if (ImGui::InputText(txt_label.c_str(), buffer, 1024)) {
            val = std::string(buffer);
          }
        } break;

        case value_type::OPAQUE_HANDLE: {
          void* handle = val;
          std::string handle_str = std::format("<opaque-handle {:p}>", handle);
          ImGui::Text("%s", handle_str.c_str());
        } break;
        case value_type::USER_TYPE: {
          void* data = val.get_mutable_storage().data();
          std::string data_str = std::format("<user-type @ {:p}, size={}>", data, val.size());
          ImGui::Text("%s", data_str.c_str());
        } break;
        default: {
          value_type val_type = val.type();
          switch (val_type) {
            case value_type::OEBOOL:
            case value_type::INT8:
            case value_type::INT16:
            case value_type::INT32:
            case value_type::INT64:
            case value_type::UINT8:
            case value_type::UINT16:
            case value_type::UINT32:
            case value_type::UINT64:
            case value_type::FLOAT:
            case value_type::DOUBLE: {
              ImGuiDataType type_enum = ImGuiDataType_COUNT;
              switch (val.type()) {
                case value_type::OEBOOL: type_enum = ImGuiDataType_S8; break;
                case value_type::INT8: type_enum = ImGuiDataType_S8; break;
                case value_type::INT16: type_enum = ImGuiDataType_S16; break;
                case value_type::INT32: type_enum = ImGuiDataType_S32; break;
                case value_type::INT64: type_enum = ImGuiDataType_S64; break;
                case value_type::UINT8: type_enum = ImGuiDataType_U8; break;
                case value_type::UINT16: type_enum = ImGuiDataType_U16; break;
                case value_type::UINT32: type_enum = ImGuiDataType_U32; break;
                case value_type::UINT64: type_enum = ImGuiDataType_U64; break;
                case value_type::FLOAT: type_enum = ImGuiDataType_Float; break;
                case value_type::DOUBLE: type_enum = ImGuiDataType_Double; break;
                default: break;
              }

              OTHER_ASSERT(type_enum != ImGuiDataType_COUNT, "Invalid ImGui data type enum for scalar value editor");

              /// \todo get mins, maxes for appropriate types?
              auto drag_scalar = [&]<typename T>() -> bool {
                T temp = val;
                std::string drag_label = std::format("{}##scalar-editor-{}", label, typeid(T).name());
                if (ImGui::DragScalar(drag_label.c_str(), type_enum, &temp, 0.1f, nullptr, nullptr, nullptr, 0)) {
                  val = temp;
                  return true;
                }
                return false;
              };

              switch (type_enum) {
                case ImGuiDataType_S8: drag_scalar.template operator()<int8_t>(); break;
                case ImGuiDataType_S16: drag_scalar.template operator()<int16_t>(); break;
                case ImGuiDataType_S32: drag_scalar.template operator()<int32_t>(); break;
                case ImGuiDataType_S64: drag_scalar.template operator()<int64_t>(); break;
                case ImGuiDataType_U8: drag_scalar.template operator()<uint8_t>(); break;
                case ImGuiDataType_U16: drag_scalar.template operator()<uint16_t>(); break;
                case ImGuiDataType_U32: drag_scalar.template operator()<uint32_t>(); break;
                case ImGuiDataType_U64: drag_scalar.template operator()<uint64_t>(); break;
                case ImGuiDataType_Float: drag_scalar.template operator()<float>(); break;
                case ImGuiDataType_Double: drag_scalar.template operator()<double>(); break;
                default: break;
              }
            } break;

            case value_type::IVEC2: ui::edit_ivec2(label, val); break;
            case value_type::IVEC3: ui::edit_ivec3(label, val); break;
            case value_type::IVEC4: ui::edit_ivec4(label, val); break;
            case value_type::VEC2: ui::edit_vec2(label, val); break;
            case value_type::VEC3: ui::edit_vec3(label, val); break;
            case value_type::VEC4: ui::edit_vec4(label, val); break;

            default:
              break;
          }
        } break;
      }

      return changed;
    }

    void draw_value_memory(const char* label, value& val, bool with_options, bool show_ascii, uint32_t columns) {
      PROFILE_SECTION("draw_value_memory");
      auto& raw_data = val.get_mutable_storage();

      static MemoryEditor mem_edit;
      mem_edit.OptShowOptions = with_options;
      mem_edit.OptShowHexII = true;
      mem_edit.OptShowAscii = show_ascii;
      mem_edit.Cols = columns;

      mem_edit.DrawContents(raw_data.data(), raw_data.size());
    }

    void value_editor::on_render_node_body() {
      PROFILE_SECTION("value_editor::on_render_node_body");
      ImVec2 base_position = ImGui::GetCursorScreenPos();
      ImVec2 window_max = {
        base_position.x + ImGui::GetContentRegionAvail().x,
        base_position.y + ImGui::GetContentRegionAvail().y
      };
      ImVec2 padding = ImVec2(1.5f, 1.5f);

      ImRect window_bg_rect = ImRect(base_position, window_max);
      ImRect inner_rect = ImRect(
        ImVec2(window_bg_rect.Min.x + padding.x, window_bg_rect.Min.y + padding.y),
        ImVec2(window_bg_rect.Max.x - padding.x, window_bg_rect.Max.y - padding.y));

      ImRect titlebar_rect = ImRect(
        ImVec2(window_bg_rect.Min.x, window_bg_rect.Min.y),
        ImVec2(window_bg_rect.Max.x, window_bg_rect.Min.y + ImGui::GetFrameHeight()));

      std::string type_name = get_value_type_string_from_type(stored_value.type());
      std::string fmt_str = std::format("[{} : {} bytes]", type_name, stored_value.size());
      std::string calc_title_text = calculate_display_text(fmt_str, ImGui::GetContentRegionAvail().x - 15.f);
      float text_width = ImGui::CalcTextSize(calc_title_text.c_str()).x;

      auto* draw_list = ImGui::GetWindowDrawList();

      /// header
      ImVec4 col = colors::rgba_to_imvec4(colors::kNodeEditorBackground);
      ImVec4 tb_col = colors::rgba_to_imvec4(colors::kNodeHeaderColor);
      ImVec2 text_pos = ImVec2{ titlebar_rect.Min.x + (titlebar_rect.GetWidth() - text_width) / 2, titlebar_rect.Min.y + 2.0f };
      draw_list->AddRectFilled(window_bg_rect.Min, window_bg_rect.Max, ImGui::GetColorU32(col));
      draw_list->AddRectFilled(titlebar_rect.Min, titlebar_rect.Max, ImGui::GetColorU32(tb_col));
      draw_list->AddText(text_pos, colors::rgba_to_hex(colors::kTextValueData), calc_title_text.c_str());

      /// body
      col.w = 1.f;
      ImVec2 body_start = ImVec2(inner_rect.Min.x, titlebar_rect.Max.y + padding.y);
      ImVec2 body_end = ImVec2(inner_rect.Max.x, inner_rect.Max.y - padding.y);
      ImVec2 inner_body_start = ImVec2(body_start.x + padding.x, body_start.y + padding.y);
      ImVec2 inner_body_end = ImVec2(body_end.x - padding.x, body_end.y - padding.y);
      ImRect body_rect = ImRect(inner_body_start, inner_body_end);
      draw_list->AddRectFilled(body_rect.Min, body_rect.Max, ImGui::GetColorU32(col));

      ImGui::SetCursorPos(ImVec2{ inner_body_start.x - ImGui::GetWindowPos().x, inner_body_start.y - ImGui::GetWindowPos().y });

      /// value editor
      float halfway_x = (body_rect.Min.x + body_rect.Max.x) / 2.0f;

      /// left half
      ImRect value_rect = ImRect(
        ImVec2(body_rect.Min.x + padding.x, body_rect.Min.y + padding.y),
        ImVec2(halfway_x - padding.x, body_rect.Max.y - padding.y));
      /// right half
      ImRect raw_memory_rect = ImRect(
        ImVec2(halfway_x + padding.x, body_rect.Min.y + padding.y),
        ImVec2(body_rect.Max.x - padding.x, body_rect.Max.y - padding.y));

      std::string child_label = std::format("##ValueEditorValue:{}", node_title);
      if (ImGui::BeginChild(child_label.c_str(), ImVec2(value_rect.GetWidth(), value_rect.GetHeight()))) {
        std::string label = std::format("Value Editor##{}", node_title);
        ui::edit_value(label.c_str(), stored_value);
      }
      ImGui::EndChild();

      ImGui::SetNextWindowPos(ImVec2{ raw_memory_rect.Min.x, raw_memory_rect.Min.y }, ImGuiCond_Always);

      child_label = std::format("##ValueEditorRawMemory:{}", node_title);
      if (ImGui::BeginChild(child_label.c_str(), ImVec2(raw_memory_rect.GetWidth(), raw_memory_rect.GetHeight()))) {
        child_label = std::format("Raw Memory##{}", node_title);
        ui::draw_value_memory(child_label.c_str(), stored_value);
      }
      ImGui::EndChild();

      // /// memory half
      // auto& raw_data = test_value.get_mutable_storage();
    }

  }  // namespace ui
}  // namespace other