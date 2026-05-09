/**
 * \file scripting/dotnet_bindings/ui_bindings.cpp
 **/
#include "scripting/dotnet_bindings/ui_bindings.hpp"

#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    bool native_begin_window(native_string title, int32_t flags) {
      std::string title_str = title;
      return ImGui::Begin(title_str.c_str(), nullptr, static_cast<ImGuiWindowFlags>(flags));
    }

    void native_end_window() {
      ImGui::End();
    }

    bool native_begin_child(native_string str_id, ImVec2 size, bool border, int32_t flags) {
      std::string id = str_id;
      return ImGui::BeginChild(id.c_str(), size, border, static_cast<ImGuiChildFlags>(flags));
    }

    void native_end_child() {
      ImGui::EndChild();
    }

    void native_ui_text(native_string text) {
      std::string text_str = text;
      ImGui::TextUnformatted(text_str.c_str());
    }

    void native_ui_text_colored(float r, float g, float b, float a, native_string text) {
      std::string text_str = text;
      ImGui::TextColored(ImVec4(r, g, b, a), "%s", text_str.c_str());
    }

    void native_ui_text_wrapped(native_string text) {
      std::string text_str = text;
      ImGui::TextWrapped("%s", text_str.c_str());
    }

    void native_ui_label_text(native_string label, native_string text) {
      std::string label_str = label;
      std::string text_str = text;
      ImGui::LabelText(label_str.c_str(), "%s", text_str.c_str());
    }

    nbool32 native_ui_button(native_string label, float width, float height) {
      std::string label_str = label;
      return ImGui::Button(label_str.c_str(), ImVec2(width, height));
    }

    nbool32 native_ui_small_button(native_string label) {
      std::string label_str = label;
      return ImGui::SmallButton(label_str.c_str());
    }

    nbool32 native_ui_invisible_button(native_string str_id, float width, float height) {
      std::string id = str_id;
      return ImGui::InvisibleButton(id.c_str(), ImVec2(width, height));
    }

    nbool32 native_ui_checkbox(native_string label, nbool32* value) {
      std::string label_str = label;
      bool v = *value;
      bool changed = ImGui::Checkbox(label_str.c_str(), &v);
      *value = v;
      return changed;
    }

    nbool32 native_ui_radio_button(native_string label, nbool32 active) {
      std::string label_str = label;
      return ImGui::RadioButton(label_str.c_str(), (bool)active);
    }

    void native_ui_progress_bar(float fraction, float width, float height, native_string overlay) {
      std::string overlay_str = overlay;
      const char* overlay_ptr = overlay_str.empty() ? nullptr : overlay_str.c_str();
      ImGui::ProgressBar(fraction, ImVec2(width, height), overlay_ptr);
    }

    void native_ui_separator() {
      ImGui::Separator();
    }

    void native_ui_same_line(float offset, float spacing) {
      ImGui::SameLine(offset, spacing);
    }

    void native_ui_spacing() {
      ImGui::Spacing();
    }

    void native_ui_indent(float width) {
      ImGui::Indent(width);
    }

    void native_ui_unindent(float width) {
      ImGui::Unindent(width);
    }

    void native_ui_new_line() {
      ImGui::NewLine();
    }

    void native_ui_dummy(float width, float height) {
      ImGui::Dummy(ImVec2(width, height));
    }

    nbool32 native_ui_input_text(native_string label, native_string buffer, int32_t buffer_size, int32_t flags) {
      std::string label_str = label;
      std::string buffer_str = buffer;
      char* buf = (char*)(const char*)buffer_str.c_str();
      return ImGui::InputText(label_str.c_str(), buf, buffer_size, static_cast<ImGuiInputTextFlags>(flags));
    }

    nbool32 native_ui_input_text_multiline(native_string label, native_string buffer, int32_t buffer_size, float width, float height, int32_t flags) {
      std::string label_str = label;
      std::string buffer_str = buffer;
      char* buf = (char*)(const char*)buffer_str.c_str();
      return ImGui::InputTextMultiline(label_str.c_str(), buf, buffer_size, ImVec2(width, height), static_cast<ImGuiInputTextFlags>(flags));
    }

    nbool32 native_ui_input_float(native_string label, float* value, float step, float step_fast, int32_t decimal_precision) {
      std::string label_str = label;
      std::string format = std::format("%.{}f", decimal_precision);
      return ImGui::InputFloat(label_str.c_str(), value, step, step_fast, format.c_str());
    }

    nbool32 native_ui_input_float2(native_string label, float* values) {
      std::string label_str = label;
      return ImGui::InputFloat2(label_str.c_str(), values);
    }

    nbool32 native_ui_input_float3(native_string label, float* values) {
      std::string label_str = label;
      return ImGui::InputFloat3(label_str.c_str(), values);
    }

    nbool32 native_ui_input_float4(native_string label, float* values) {
      std::string label_str = label;
      return ImGui::InputFloat4(label_str.c_str(), values);
    }

    nbool32 native_ui_input_int(native_string label, int32_t* value, int32_t step, int32_t step_fast) {
      std::string label_str = label;
      return ImGui::InputInt(label_str.c_str(), value, step, step_fast);
    }

    nbool32 native_ui_drag_float(native_string label, float* value, float speed, float min_val, float max_val) {
      std::string label_str = label;
      return ImGui::DragFloat(label_str.c_str(), value, speed, min_val, max_val);
    }

    nbool32 native_ui_drag_float3(native_string label, float* values, float speed, float min_val, float max_val) {
      std::string label_str = label;
      return ImGui::DragFloat3(label_str.c_str(), values, speed, min_val, max_val);
    }

    nbool32 native_ui_slider_float(native_string label, float* value, float min_val, float max_val) {
      std::string label_str = label;
      return ImGui::SliderFloat(label_str.c_str(), value, min_val, max_val);
    }

    nbool32 native_ui_slider_int(native_string label, int32_t* value, int32_t min_val, int32_t max_val) {
      std::string label_str = label;
      return ImGui::SliderInt(label_str.c_str(), value, min_val, max_val);
    }

    nbool32 native_ui_color_edit3(native_string label, float* col) {
      std::string label_str = label;
      return ImGui::ColorEdit3(label_str.c_str(), col);
    }

    nbool32 native_ui_color_edit4(native_string label, float* col) {
      std::string label_str = label;
      return ImGui::ColorEdit4(label_str.c_str(), col);
    }

    nbool32 native_ui_tree_node(native_string label) {
      std::string label_str = label;
      return ImGui::TreeNode(label_str.c_str());
    }

    nbool32 native_ui_tree_node_ex(native_string label, int32_t flags) {
      std::string label_str = label;
      return ImGui::TreeNodeEx(label_str.c_str(), static_cast<ImGuiTreeNodeFlags>(flags));
    }

    void native_ui_tree_pop() {
      ImGui::TreePop();
    }

    nbool32 native_ui_collapsing_header(native_string label, int32_t flags) {
      std::string label_str = label;
      return ImGui::CollapsingHeader(label_str.c_str(), static_cast<ImGuiTreeNodeFlags>(flags));
    }

    nbool32 native_ui_selectable(native_string label, nbool32 selected, int32_t flags) {
      std::string label_str = label;
      return ImGui::Selectable(label_str.c_str(), (bool)selected, static_cast<ImGuiSelectableFlags>(flags));
    }

    nbool32 native_ui_begin_combo(native_string label, native_string preview_value, int32_t flags) {
      std::string label_str = label;
      std::string preview_str = preview_value;
      return ImGui::BeginCombo(label_str.c_str(), preview_str.c_str(), static_cast<ImGuiComboFlags>(flags));
    }

    void native_ui_end_combo() {
      ImGui::EndCombo();
    }

    nbool32 native_ui_begin_listbox(native_string label, float width, float height) {
      std::string label_str = label;
      return ImGui::BeginListBox(label_str.c_str(), ImVec2(width, height));
    }

    void native_ui_end_listbox() {
      ImGui::EndListBox();
    }

    nbool32 native_ui_begin_tab_bar(native_string str_id, int32_t flags) {
      std::string id = str_id;
      return ImGui::BeginTabBar(id.c_str(), static_cast<ImGuiTabBarFlags>(flags));
    }

    void native_ui_end_tab_bar() {
      ImGui::EndTabBar();
    }

    nbool32 native_ui_begin_tab_item(native_string label, int32_t flags) {
      std::string label_str = label;
      return ImGui::BeginTabItem(label_str.c_str(), nullptr, static_cast<ImGuiTabItemFlags>(flags));
    }

    void native_ui_end_tab_item() {
      ImGui::EndTabItem();
    }

    nbool32 native_ui_begin_menu_bar() {
      return ImGui::BeginMenuBar();
    }

    void native_ui_end_menu_bar() {
      ImGui::EndMenuBar();
    }

    nbool32 native_ui_begin_main_menu_bar() {
      return ImGui::BeginMainMenuBar();
    }

    void native_ui_end_main_menu_bar() {
      ImGui::EndMainMenuBar();
    }

    nbool32 native_ui_begin_menu(native_string label, nbool32 enabled) {
      std::string label_str = label;
      return ImGui::BeginMenu(label_str.c_str(), (bool)enabled);
    }

    void native_ui_end_menu() {
      ImGui::EndMenu();
    }

    nbool32 native_ui_menu_item(native_string label, native_string shortcut, nbool32 selected, nbool32 enabled) {
      std::string label_str = label;
      std::string shortcut_str = shortcut;
      const char* shortcut_ptr = shortcut_str.empty() ? nullptr : shortcut_str.c_str();
      return ImGui::MenuItem(label_str.c_str(), shortcut_ptr, (bool)selected, (bool)enabled);
    }

    void native_ui_open_popup(native_string str_id) {
      std::string id = str_id;
      ImGui::OpenPopup(id.c_str());
    }

    nbool32 native_ui_begin_popup(native_string str_id, int32_t flags) {
      std::string id = str_id;
      return ImGui::BeginPopup(id.c_str(), static_cast<ImGuiWindowFlags>(flags));
    }

    nbool32 native_ui_begin_popup_modal(native_string name, int32_t flags) {
      std::string name_str = name;
      return ImGui::BeginPopupModal(name_str.c_str(), nullptr, static_cast<ImGuiWindowFlags>(flags));
    }

    void native_ui_end_popup() {
      ImGui::EndPopup();
    }

    void native_ui_close_current_popup() {
      ImGui::CloseCurrentPopup();
    }

    nbool32 native_ui_begin_table(native_string str_id, int32_t columns, int32_t flags) {
      std::string id = str_id;
      return ImGui::BeginTable(id.c_str(), columns, static_cast<ImGuiTableFlags>(flags));
    }

    void native_ui_end_table() {
      ImGui::EndTable();
    }

    void native_ui_table_next_row(int32_t flags, float min_row_height) {
      ImGui::TableNextRow(static_cast<ImGuiTableRowFlags>(flags), min_row_height);
    }

    nbool32 native_ui_table_next_column() {
      return ImGui::TableNextColumn();
    }

    nbool32 native_ui_table_set_column_index(int32_t column_n) {
      return ImGui::TableSetColumnIndex(column_n);
    }

    void native_ui_table_setup_column(native_string label, int32_t flags, float init_width_or_weight) {
      std::string label_str = label;
      ImGui::TableSetupColumn(label_str.c_str(), static_cast<ImGuiTableColumnFlags>(flags), init_width_or_weight);
    }

    void native_ui_table_headers_row() {
      ImGui::TableHeadersRow();
    }

    void native_ui_get_content_region_avail(float* out_x, float* out_y) {
      ImVec2 avail = ImGui::GetContentRegionAvail();
      *out_x = avail.x;
      *out_y = avail.y;
    }

    void native_ui_get_window_size(float* out_width, float* out_height) {
      ImVec2 size = ImGui::GetWindowSize();
      *out_width = size.x;
      *out_height = size.y;
    }

    void native_ui_get_window_pos(float* out_x, float* out_y) {
      ImVec2 pos = ImGui::GetWindowPos();
      *out_x = pos.x;
      *out_y = pos.y;
    }

    void native_ui_set_next_window_size(float width, float height, int32_t cond) {
      ImGui::SetNextWindowSize(ImVec2(width, height), cond);
    }

    void native_ui_set_next_window_pos(float x, float y, int32_t cond) {
      ImGui::SetNextWindowPos(ImVec2(x, y), cond);
    }

    nbool32 native_ui_is_item_hovered() {
      return ImGui::IsItemHovered();
    }

    nbool32 native_ui_is_item_clicked(int32_t mouse_button) {
      return ImGui::IsItemClicked(mouse_button);
    }

    nbool32 native_ui_is_item_active() {
      return ImGui::IsItemActive();
    }

    nbool32 native_ui_is_window_focused(int32_t flags) {
      return ImGui::IsWindowFocused(static_cast<ImGuiFocusedFlags>(flags));
    }

    nbool32 native_ui_is_window_hovered(int32_t flags) {
      return ImGui::IsWindowHovered(static_cast<ImGuiHoveredFlags>(flags));
    }

    void native_ui_push_style_color(int32_t idx, float r, float g, float b, float a) {
      ImGui::PushStyleColor(idx, ImVec4(r, g, b, a));
    }

    void native_ui_pop_style_color(int32_t count) {
      ImGui::PopStyleColor(count);
    }

    void native_ui_push_style_var_float(int32_t idx, float val) {
      ImGui::PushStyleVar(idx, val);
    }

    void native_ui_push_style_var_vec2(int32_t idx, float x, float y) {
      ImGui::PushStyleVar(idx, ImVec2(x, y));
    }

    void native_ui_pop_style_var(int32_t count) {
      ImGui::PopStyleVar(count);
    }

    void native_ui_push_item_width(float width) {
      ImGui::PushItemWidth(width);
    }

    void native_ui_pop_item_width() {
      ImGui::PopItemWidth();
    }

    void native_ui_push_id_str(native_string str_id) {
      std::string id = str_id;
      ImGui::PushID(id.c_str());
    }

    void native_ui_push_id_int(int32_t int_id) {
      ImGui::PushID(int_id);
    }

    void native_ui_pop_id() {
      ImGui::PopID();
    }

    void native_ui_set_tooltip(native_string text) {
      std::string text_str = text;
      ImGui::SetTooltip("%s", text_str.c_str());
    }

    nbool32 native_ui_begin_tooltip() {
      return ImGui::BeginTooltip();
    }

    void native_ui_end_tooltip() {
      ImGui::EndTooltip();
    }

    nbool32 native_ui_begin_drag_drop_source(int32_t flags) {
      return ImGui::BeginDragDropSource(static_cast<ImGuiDragDropFlags>(flags));
    }

    nbool32 native_ui_set_drag_drop_payload(native_string type, void* data, int32_t size) {
      std::string type_str = type;
      return ImGui::SetDragDropPayload(type_str.c_str(), data, size);
    }

    void native_ui_end_drag_drop_source() {
      ImGui::EndDragDropSource();
    }

    nbool32 native_ui_begin_drag_drop_target() {
      return ImGui::BeginDragDropTarget();
    }

    void native_ui_end_drag_drop_target() {
      ImGui::EndDragDropTarget();
    }

  }  // namespace bindings
}  // namespace other