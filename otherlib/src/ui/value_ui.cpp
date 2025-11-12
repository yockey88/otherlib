/**
 * \file ui/value_ui.cpp
 **/
#include "ui/value_ui.hpp"

#include <winnt.h>

#include "renderer/ui/ui_helpers.hpp"

#include "ui/colors.hpp"
#include "ui/ui_widgets.hpp"

#include "colors.hpp"

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

    void value_editor(const char* label, value& val) {
      std::string type_name = get_value_type_string_from_type(val.type());
      {
        scoped_color text_color(ImGuiCol_Text, colors::kTextValueData);
        std::string fmt_str = std::format("{} [{} : {} bytes]", label, type_name, val.size());
        std::string display_text = calculate_display_text(fmt_str, ImGui::GetContentRegionAvail().x - 15.f);
        ImGui::Text("%s", display_text.c_str());
      }

      bool string = val.type() == value_type::STRING;
      bool opaque = val.type() == value_type::OPAQUE_HANDLE;
      bool user_defined = val.type() == value_type::USER_TYPE;

      bool scalar = false;
      bool lin_alg_type = (val.type() >= value_type::VEC2 && val.type() <= value_type::MAT4);
      ImGuiDataType type_enum = ImGuiDataType_COUNT;
      {
        scoped_color text_color(ImGuiCol_Text, colors::kText);
        if (string) {
          std::string str_val = val;

          char buffer[1024];
          std::ranges::fill(buffer, 0);
          std::strncpy(buffer, str_val.c_str(), sizeof(buffer));
          if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
            val = std::string(buffer);
          }
        } else if (opaque) {
          ImGui::Text("<opaque-handle> @ {:p}", val.get_mutable_storage().data());
        } else if (user_defined) {
          ImGui::Text("<user-defined-type> @ {:p}", val.get_mutable_storage().data());
        } else {
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

          scalar = type_enum != ImGuiDataType_COUNT;
          lin_alg_type = (val.type() >= value_type::VEC2 && val.type() <= value_type::MAT4);
        }

        if (!scalar && !lin_alg_type) {
          return;
        }

        if (scalar) {
          auto drag_scalar = [&]<typename T>() {
            T temp = val;
            if (ImGui::DragScalar("##value", type_enum, &temp, 0.1f, nullptr, nullptr, nullptr, 0)) {
              val = temp;
            }
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
        } else if (lin_alg_type) {
          switch (val.type()) {
            case value_type::IVEC2: break;
            case value_type::IVEC3: break;
            case value_type::IVEC4: break;

            case value_type::VEC2: break;
            case value_type::VEC3: break;
            case value_type::VEC4: break;
            default: {
              scoped_color error_color(ImGuiCol_Text, colors::kTextError);
              ImGui::Text("Unsupported linear algebra type for value editor");
            } break;
          }
        }

      }  // namespace ui
    }  // namespace other