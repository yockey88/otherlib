/**
 * \file ui/type-bindings/value_ui.hpp
 **/
#ifndef OTHERLIB_UI_TYPE_BINDINGS_VALUE_UI_HPP
#define OTHERLIB_UI_TYPE_BINDINGS_VALUE_UI_HPP

#include <cstdint>

#include <imgui/ImReflect.hpp>
#include <imgui/imgui.h>

#include "core/value.hpp"

#include "ui/ui_node.hpp"

namespace other {
  namespace ui {

    void begin_property(const char* name, const char* help_text = "");
    void end_property();

    void begin_property_grid(uint32_t columns = 2, const ImVec2& item_spacing = ImVec2(8.f, 8.f), const ImVec2& frame_pading = ImVec2(4.f, 4.f));
    void end_property_grid();

    std::string edit_string(const char* label, const std::string& val);
    bool edit_value(const char* label, value& val);

    void draw_value_memory(const char* label, value& val, bool with_options = false, bool show_ascii = true, uint32_t columns = 8);

    struct value_editor : public ui_node {
      value stored_value;

      value_editor(const value& value, ui_window* parent, const std::string& name)
          : ui_node(parent, std::format("Value {}", name)), stored_value(value) {}
      virtual ~value_editor() override = default;

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_TYPE_BINDINGS_VALUE_UI_HPP