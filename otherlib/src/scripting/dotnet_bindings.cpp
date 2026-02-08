/**
 * \file scripting/dotnet_bindings.cpp
 **/
#include "scripting/dotnet_bindings.hpp"

#include <imgui/imgui.h>

#include "script/scripting_environment.hpp"

#include "object/scene_object.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace bindings {
    namespace {

      static driver* s_driver = nullptr;

      scene* get_active_scene_checked() {
        OTHER_ASSERT(s_driver != nullptr, "Driver pointer is null.");
        auto* env = subsystem<scripting_environment>::get();
        OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");
        auto* active_scene = s_driver->get_active_scene();
        OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
        return active_scene;
      }

    }  // namespace

    native_string native_get_program_files_folder(native_string app_name_str) {
      std::string app_name = app_name_str;
      filepath program_files_folder = get_program_files_folder(app_name);
      return native_string::new_str(program_files_folder.string());
    }

    native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag) {
      std::string app_name = app_name_str;
      filepath app_data_folder = get_app_data_folder(app_name, create_flag != 0);
      return native_string::new_str(app_data_folder.string());
    }

    native_string native_get_install_folder() {
      filepath install_folder = get_other_environment_install_folder();
      return native_string::new_str(install_folder.string());
    }

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

    void native_get_object_id(void* object_ptr, natural_t* out_id) {
      OTHER_ASSERT(object_ptr != nullptr, "Native object pointer is null.");
      OTHER_ASSERT(out_id != nullptr, "Output ID pointer is null.");

      scripting_environment* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");

      *out_id = ((scene_object*)object_ptr)->id;
    }

    natural_t native_scene_create_object(native_string name, float x, float y, float z) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        CORE_LOG_ERROR("No active scene to create object in.");
        return 0;
      }
      std::string name_str = name;
      scene_object& obj = active_scene->create_object(name_str, glm::vec3(x, y, z));
      return obj.id;
    }

    void native_scene_destroy_object(natural_t id) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->destroy_object(id);
    }

    nbool32 native_scene_has_object(natural_t id) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->has_object(id);
    }

    native_string native_scene_get_object_name(natural_t id) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return native_string::new_str("");
      }
      const scene_object& obj = active_scene->get_object(id);
      return native_string::new_str(obj.name);
    }

    void native_scene_set_object_name(natural_t id, native_string name) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      scene_object& obj = active_scene->get_object(id);
      obj.name = (std::string)name;
    }

    void native_scene_get_object_ids(natural_t* out_ids, int32_t* out_count, int32_t max_count) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        *out_count = 0;
        return;
      }
      auto ids = active_scene->get_all_object_ids();
      int32_t count = std::min((int32_t)ids.size(), max_count);
      for (int32_t i = 0; i < count; ++i) {
        out_ids[i] = ids[i];
      }
      *out_count = count;
    }

    natural_t native_scene_get_object_count() {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      return active_scene->get_object_count();
    }

    natural_t native_scene_find_object_by_name(native_string name) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      std::string name_str = name;
      scene_object* obj = active_scene->find_object(name_str);
      if (obj == nullptr) {
        return 0;
      }
      return obj->id;
    }

    natural_t native_scene_get_parent_id(natural_t id) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      const scene_object* parent = active_scene->get_parent(id);
      return parent != nullptr ? parent->id : 0;
    }

    void native_scene_get_children_ids(natural_t id, natural_t* out_ids, int32_t* out_count, int32_t max_count) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        *out_count = 0;
        return;
      }
      auto ids = active_scene->get_children_ids(id);
      int32_t count = std::min((int32_t)ids.size(), max_count);
      for (int32_t i = 0; i < count; ++i) {
        out_ids[i] = ids[i];
      }
      *out_count = count;
    }

    nbool32 native_scene_object_has_tag(natural_t id, native_string tag) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->object_has_tag(id, (std::string)tag);
    }

    void native_scene_add_object_tag(natural_t id, native_string tag) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->add_object_tag(id, (std::string)tag);
    }

    void native_scene_remove_object_tag(natural_t id, native_string tag) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->remove_object_tag(id, (std::string)tag);
    }

    nbool32 native_scene_get_object_visible(natural_t id) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->get_object(id).visible;
    }

    void native_scene_set_object_visible(natural_t id, nbool32 visible) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->get_object(id).visible = visible;
    }

    void native_transform_get_position(natural_t id, float* out_x, float* out_y, float* out_z) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_position.x;
      *out_y = t.local_position.y;
      *out_z = t.local_position.z;
    }

    void native_transform_set_position(natural_t id, float x, float y, float z) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform t = active_scene->get_transform(id);
      t.local_position = glm::vec3(x, y, z);
      active_scene->set_transform(id, t);
    }

    void native_transform_get_rotation(natural_t id, float* out_x, float* out_y, float* out_z, float* out_w) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_rotation_quat.x;
      *out_y = t.local_rotation_quat.y;
      *out_z = t.local_rotation_quat.z;
      *out_w = t.local_rotation_quat.w;
    }

    void native_transform_set_rotation(natural_t id, float x, float y, float z, float w) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform t = active_scene->get_transform(id);
      t.local_rotation_quat = glm::quat(w, x, y, z);
      active_scene->set_transform(id, t);
    }

    void native_transform_get_scale(natural_t id, float* out_x, float* out_y, float* out_z) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      const transform& t = active_scene->get_transform(id);
      *out_x = t.local_scale.x;
      *out_y = t.local_scale.y;
      *out_z = t.local_scale.z;
    }

    void native_transform_set_scale(natural_t id, float x, float y, float z) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      transform t = active_scene->get_transform(id);
      t.local_scale = glm::vec3(x, y, z);
      active_scene->set_transform(id, t);
    }

    void native_transform_get_world_matrix(natural_t id, float* out_matrix) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      glm::mat4 world = active_scene->get_world_transform(id);
      std::memcpy(out_matrix, &world[0][0], sizeof(float) * 16);
    }

    void native_component_add_by_name(integer_t id, native_string component_name) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) return;
      scene_object& obj = active_scene->get_object(id);
      active_scene->add_component_by_name(&obj, (std::string)component_name);
    }

    void native_component_remove_by_name(integer_t id, native_string component_name) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) return;
      scene_object& obj = active_scene->get_object(id);
      active_scene->remove_component_by_name(&obj, (std::string)component_name);
    }

    nbool32 native_component_has_by_name(integer_t id, native_string component_name) {
      scene* active_scene = get_active_scene_checked();
      if (active_scene == nullptr) return false;
      scene_object& obj = active_scene->get_object(id);
      return active_scene->has_component_by_name(&obj, (std::string)component_name);
    }

    // ===================================================================
    //  Events
    // ===================================================================

    void native_event_register(native_string event_name) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_register not yet connected to event system");
    }

    void native_event_trigger(native_string event_name) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_trigger not yet connected to event system");
    }

    void native_event_trigger_with_string(native_string event_name, native_string data) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_trigger_with_string not yet connected to event system");
    }

    // ===================================================================
    //  Input
    // ===================================================================

    nbool32 native_input_is_key_down(int32_t sdl_scancode) {
      const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
      if (keyboard_state == nullptr) return false;
      return keyboard_state[sdl_scancode];
    }

    nbool32 native_input_is_key_pressed(int32_t sdl_scancode) {
      /// \note ImGui tracks pressed (transition) state, SDL only gives current state
      ///       we use ImGui's key mapping for pressed detection
      ImGuiKey imgui_key = static_cast<ImGuiKey>(sdl_scancode);
      return ImGui::IsKeyPressed(imgui_key);
    }

    nbool32 native_input_is_mouse_button_down(int32_t button) {
      return ImGui::IsMouseDown(button);
    }

    nbool32 native_input_is_mouse_button_clicked(int32_t button) {
      return ImGui::IsMouseClicked(button);
    }

    void native_input_get_mouse_position(float* out_x, float* out_y) {
      ImVec2 pos = ImGui::GetMousePos();
      *out_x = pos.x;
      *out_y = pos.y;
    }

    void native_input_get_mouse_delta(float* out_x, float* out_y) {
      ImGuiIO& io = ImGui::GetIO();
      *out_x = io.MouseDelta.x;
      *out_y = io.MouseDelta.y;
    }

    float native_input_get_mouse_wheel() {
      ImGuiIO& io = ImGui::GetIO();
      return io.MouseWheel;
    }

    float native_time_get_delta_time() {
      ImGuiIO& io = ImGui::GetIO();
      return io.DeltaTime;
    }

    float native_time_get_elapsed_time() {
      /// \todo replace with engine time accumulator
      return (float)ImGui::GetTime();
    }

    int64_t native_time_get_frame_count() {
      return ImGui::GetFrameCount();
    }

    int32_t native_driver_get_state() {
      /// \todo connect to actual driver state machine
      return 0;
    }

    void native_driver_request_shutdown() {
      /// \todo connect to driver shutdown request
      CORE_LOG_WARN("native_driver_request_shutdown: not yet connected to driver");
    }

    native_string native_driver_get_project_name() {
      /// \todo connect to driver project name
      return native_string::new_str("Unknown");
    }

    native_string native_config_get_string(native_string section, native_string key, native_string default_value) {
      /// \todo connect to config table
      return default_value;
    }

    int32_t native_config_get_int(native_string section, native_string key, int32_t default_value) {
      return default_value;
    }

    float native_config_get_float(native_string section, native_string key, float default_value) {
      return default_value;
    }

    nbool32 native_config_get_bool(native_string section, native_string key, nbool32 default_value) {
      return default_value;
    }

    nbool32 native_network_is_connected() {
      /// \todo connect to network subsystem
      return false;
    }

    int32_t native_network_get_role() {
      /// \todo connect to driver role
      return 0;
    }

    template <typename Fn>
    void bind_function(dotnet_host& dn_host, native_string name, Fn fn) {
      PROFILE_SECTION("other::bindings::bind-function");
      void* fn_ptr = (void*)fn;
      dn_host.interop().bind_native_function(name, fn_ptr);
    }

    void validate_binding_points(dotnet_host& dn_host) {
      PROFILE_SECTION("other::bindings::validate-binding-points");
      nbool32 res = dn_host.interop().validate_binding_points();
      if (!res) {
        CORE_LOG_ERROR("One or more native functions failed to bind to managed counterparts.");
      }
    }

    struct binding_context {
      dotnet_host& host;

      binding_context(dotnet_host& h)
          : host(h) {}

      template <typename Fn>
      binding_context& bind(const std::string_view name, Fn fn) {
        PROFILE_SECTION("other::bindings::binding_context::bind");
        native_scoped_string fn_name = native_string::new_str(name);
        bind_function(host, fn_name, fn);
        return *this;
      }
    };

  }  // namespace bindings

  void set_dotnet_native_driver(driver* drv) {
    OTHER_ASSERT(drv != nullptr, "Driver pointer is null.");
    bindings::s_driver = drv;
  }

  void bind_otherlib_dotnet_functions(dotnet_host& dn_host) {
    PROFILE_SECTION("other::bind-otherlib-dotnet-functions");
    dn_host.rediscover_binding_points();

    bindings::binding_context{ dn_host }
      /// Filesystem.
      .bind("GetProgramFilesFolder", bindings::native_get_program_files_folder)
      .bind("GetAppDataFolder", bindings::native_get_app_data_folder)
      .bind("GetInstallFolder", bindings::native_get_install_folder);

    bindings::binding_context{ dn_host }
      /// UI.
      .bind("BeginWindow", bindings::native_begin_window)
      .bind("EndWindow", bindings::native_end_window)
      .bind("BeginChild", bindings::native_begin_child)
      .bind("EndChild", bindings::native_end_child);

    bindings::binding_context{ dn_host }
      /// UI - Widgets.
      .bind("UIText", bindings::native_ui_text)
      .bind("UITextColored", bindings::native_ui_text_colored)
      .bind("UITextWrapped", bindings::native_ui_text_wrapped)
      .bind("UILabelText", bindings::native_ui_label_text)
      .bind("UIButton", bindings::native_ui_button)
      .bind("UISmallButton", bindings::native_ui_small_button)
      .bind("UIInvisibleButton", bindings::native_ui_invisible_button)
      .bind("UICheckbox", bindings::native_ui_checkbox)
      .bind("UIRadioButton", bindings::native_ui_radio_button)
      .bind("UIProgressBar", bindings::native_ui_progress_bar)
      .bind("UISeparator", bindings::native_ui_separator)
      .bind("UISameLine", bindings::native_ui_same_line)
      .bind("UISpacing", bindings::native_ui_spacing)
      .bind("UIIndent", bindings::native_ui_indent)
      .bind("UIUnindent", bindings::native_ui_unindent)
      .bind("UINewLine", bindings::native_ui_new_line)
      .bind("UIDummy", bindings::native_ui_dummy);

    bindings::binding_context{ dn_host }
      /// UI - Input Widgets.
      .bind("UIInputText", bindings::native_ui_input_text)
      .bind("UIInputTextMultiline", bindings::native_ui_input_text_multiline)
      .bind("UIInputFloat", bindings::native_ui_input_float)
      .bind("UIInputFloat2", bindings::native_ui_input_float2)
      .bind("UIInputFloat3", bindings::native_ui_input_float3)
      .bind("UIInputFloat4", bindings::native_ui_input_float4)
      .bind("UIInputInt", bindings::native_ui_input_int)
      .bind("UIDragFloat", bindings::native_ui_drag_float)
      .bind("UIDragFloat3", bindings::native_ui_drag_float3)
      .bind("UISliderFloat", bindings::native_ui_slider_float)
      .bind("UISliderInt", bindings::native_ui_slider_int)
      .bind("UIColorEdit3", bindings::native_ui_color_edit3)
      .bind("UIColorEdit4", bindings::native_ui_color_edit4);

    bindings::binding_context{ dn_host }
      /// UI - Tree / Collapsing.
      .bind("UITreeNode", bindings::native_ui_tree_node)
      .bind("UITreeNodeEx", bindings::native_ui_tree_node_ex)
      .bind("UITreePop", bindings::native_ui_tree_pop)
      .bind("UICollapsingHeader", bindings::native_ui_collapsing_header);

    bindings::binding_context{ dn_host }
      /// UI - Selectables / Lists.
      .bind("UISelectable", bindings::native_ui_selectable)
      .bind("UIBeginCombo", bindings::native_ui_begin_combo)
      .bind("UIEndCombo", bindings::native_ui_end_combo)
      .bind("UIBeginListBox", bindings::native_ui_begin_listbox)
      .bind("UIEndListBox", bindings::native_ui_end_listbox);

    bindings::binding_context{ dn_host }
      /// UI - Tabs.
      .bind("UIBeginTabBar", bindings::native_ui_begin_tab_bar)
      .bind("UIEndTabBar", bindings::native_ui_end_tab_bar)
      .bind("UIBeginTabItem", bindings::native_ui_begin_tab_item)
      .bind("UIEndTabItem", bindings::native_ui_end_tab_item);

    bindings::binding_context{ dn_host }
      /// UI - Menus.
      .bind("UIBeginMenuBar", bindings::native_ui_begin_menu_bar)
      .bind("UIEndMenuBar", bindings::native_ui_end_menu_bar)
      .bind("UIBeginMainMenuBar", bindings::native_ui_begin_main_menu_bar)
      .bind("UIEndMainMenuBar", bindings::native_ui_end_main_menu_bar)
      .bind("UIBeginMenu", bindings::native_ui_begin_menu)
      .bind("UIEndMenu", bindings::native_ui_end_menu)
      .bind("UIMenuItem", bindings::native_ui_menu_item);

    bindings::binding_context{ dn_host }
      /// UI - Popups / Modals.
      .bind("UIOpenPopup", bindings::native_ui_open_popup)
      .bind("UIBeginPopup", bindings::native_ui_begin_popup)
      .bind("UIBeginPopupModal", bindings::native_ui_begin_popup_modal)
      .bind("UIEndPopup", bindings::native_ui_end_popup)
      .bind("UICloseCurrentPopup", bindings::native_ui_close_current_popup);

    bindings::binding_context{ dn_host }
      /// UI - Tables.
      .bind("UIBeginTable", bindings::native_ui_begin_table)
      .bind("UIEndTable", bindings::native_ui_end_table)
      .bind("UITableNextRow", bindings::native_ui_table_next_row)
      .bind("UITableNextColumn", bindings::native_ui_table_next_column)
      .bind("UITableSetColumnIndex", bindings::native_ui_table_set_column_index)
      .bind("UITableSetupColumn", bindings::native_ui_table_setup_column)
      .bind("UITableHeadersRow", bindings::native_ui_table_headers_row);

    bindings::binding_context{ dn_host }
      /// UI - Layout Queries.
      .bind("UIGetContentRegionAvail", bindings::native_ui_get_content_region_avail)
      .bind("UIGetWindowSize", bindings::native_ui_get_window_size)
      .bind("UIGetWindowPos", bindings::native_ui_get_window_pos)
      .bind("UISetNextWindowSize", bindings::native_ui_set_next_window_size)
      .bind("UISetNextWindowPos", bindings::native_ui_set_next_window_pos)
      .bind("UIIsItemHovered", bindings::native_ui_is_item_hovered)
      .bind("UIIsItemClicked", bindings::native_ui_is_item_clicked)
      .bind("UIIsItemActive", bindings::native_ui_is_item_active)
      .bind("UIIsWindowFocused", bindings::native_ui_is_window_focused)
      .bind("UIIsWindowHovered", bindings::native_ui_is_window_hovered);

    bindings::binding_context{ dn_host }
      /// UI - Style.
      .bind("UIPushStyleColor", bindings::native_ui_push_style_color)
      .bind("UIPopStyleColor", bindings::native_ui_pop_style_color)
      .bind("UIPushStyleVarFloat", bindings::native_ui_push_style_var_float)
      .bind("UIPushStyleVarVec2", bindings::native_ui_push_style_var_vec2)
      .bind("UIPopStyleVar", bindings::native_ui_pop_style_var)
      .bind("UIPushItemWidth", bindings::native_ui_push_item_width)
      .bind("UIPopItemWidth", bindings::native_ui_pop_item_width);

    bindings::binding_context{ dn_host }
      /// UI - ID Stack.
      .bind("UIPushIDStr", bindings::native_ui_push_id_str)
      .bind("UIPushIDInt", bindings::native_ui_push_id_int)
      .bind("UIPopID", bindings::native_ui_pop_id);

    bindings::binding_context{ dn_host }
      /// UI - Tooltips.
      .bind("UISetTooltip", bindings::native_ui_set_tooltip)
      .bind("UIBeginTooltip", bindings::native_ui_begin_tooltip)
      .bind("UIEndTooltip", bindings::native_ui_end_tooltip);

    bindings::binding_context{ dn_host }
      /// UI - Drag and Drop.
      .bind("UIBeginDragDropSource", bindings::native_ui_begin_drag_drop_source)
      .bind("UISetDragDropPayload", bindings::native_ui_set_drag_drop_payload)
      .bind("UIEndDragDropSource", bindings::native_ui_end_drag_drop_source)
      .bind("UIBeginDragDropTarget", bindings::native_ui_begin_drag_drop_target)
      .bind("UIEndDragDropTarget", bindings::native_ui_end_drag_drop_target);

    bindings::binding_context{ dn_host }
      /// Object/Scene
      .bind("GetObjectID", bindings::native_get_object_id)
      .bind("SceneCreateObject", bindings::native_scene_create_object)
      .bind("SceneDestroyObject", bindings::native_scene_destroy_object)
      .bind("SceneHasObject", bindings::native_scene_has_object)
      .bind("SceneGetObjectName", bindings::native_scene_get_object_name)
      .bind("SceneSetObjectName", bindings::native_scene_set_object_name)
      .bind("SceneGetObjectIds", bindings::native_scene_get_object_ids)
      .bind("SceneGetObjectCount", bindings::native_scene_get_object_count)
      .bind("SceneFindObjectByName", bindings::native_scene_find_object_by_name);

    bindings::binding_context{ dn_host }
      /// Object Hierarchy.
      .bind("SceneGetParentId", bindings::native_scene_get_parent_id)
      .bind("SceneGetChildrenIds", bindings::native_scene_get_children_ids);

    bindings::binding_context{ dn_host }
      /// Object Tags.
      .bind("SceneObjectHasTag", bindings::native_scene_object_has_tag)
      .bind("SceneAddObjectTag", bindings::native_scene_add_object_tag)
      .bind("SceneRemoveObjectTag", bindings::native_scene_remove_object_tag);

    bindings::binding_context{ dn_host }
      /// Object Visibility.
      .bind("SceneGetObjectVisible", bindings::native_scene_get_object_visible)
      .bind("SceneSetObjectVisible", bindings::native_scene_set_object_visible);

    bindings::binding_context{ dn_host }
      /// Transform.
      .bind("TransformGetPosition", bindings::native_transform_get_position)
      .bind("TransformSetPosition", bindings::native_transform_set_position)
      .bind("TransformGetRotation", bindings::native_transform_get_rotation)
      .bind("TransformSetRotation", bindings::native_transform_set_rotation)
      .bind("TransformGetScale", bindings::native_transform_get_scale)
      .bind("TransformSetScale", bindings::native_transform_set_scale)
      .bind("TransformGetWorldMatrix", bindings::native_transform_get_world_matrix);

    bindings::binding_context{ dn_host }
      /// Components.
      .bind("ComponentAddByName", bindings::native_component_add_by_name)
      .bind("ComponentRemoveByName", bindings::native_component_remove_by_name)
      .bind("ComponentHasByName", bindings::native_component_has_by_name);

    bindings::binding_context{ dn_host }
      /// Events.
      .bind("EventRegister", bindings::native_event_register)
      .bind("EventTrigger", bindings::native_event_trigger)
      .bind("EventTriggerWithString", bindings::native_event_trigger_with_string);

    bindings::binding_context{ dn_host }
      /// Input.
      .bind("InputIsKeyDown", bindings::native_input_is_key_down)
      .bind("InputIsKeyPressed", bindings::native_input_is_key_pressed)
      .bind("InputIsMouseButtonDown", bindings::native_input_is_mouse_button_down)
      .bind("InputIsMouseButtonClicked", bindings::native_input_is_mouse_button_clicked)
      .bind("InputGetMousePosition", bindings::native_input_get_mouse_position)
      .bind("InputGetMouseDelta", bindings::native_input_get_mouse_delta)
      .bind("InputGetMouseWheel", bindings::native_input_get_mouse_wheel);

    bindings::binding_context{ dn_host }
      /// Time.
      .bind("TimeGetDeltaTime", bindings::native_time_get_delta_time)
      .bind("TimeGetElapsedTime", bindings::native_time_get_elapsed_time)
      .bind("TimeGetFrameCount", bindings::native_time_get_frame_count);

    bindings::binding_context{ dn_host }
      /// Driver / Application State.
      .bind("DriverGetState", bindings::native_driver_get_state)
      .bind("DriverRequestShutdown", bindings::native_driver_request_shutdown)
      .bind("DriverGetProjectName", bindings::native_driver_get_project_name);

    bindings::binding_context{ dn_host }
      /// Configuration.
      .bind("ConfigGetString", bindings::native_config_get_string)
      .bind("ConfigGetInt", bindings::native_config_get_int)
      .bind("ConfigGetFloat", bindings::native_config_get_float)
      .bind("ConfigGetBool", bindings::native_config_get_bool);

    bindings::binding_context{ dn_host }
      /// Networking.
      .bind("NetworkIsConnected", bindings::native_network_is_connected)
      .bind("NetworkGetRole", bindings::native_network_get_role);

    bindings::validate_binding_points(dn_host);
  }

}  // namespace other