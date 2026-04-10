/**
 * \file scripting/dotnet_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP

#include "dotnet/host.hpp"

namespace other {

  class driver;

  void set_dotnet_native_driver(driver* drv);
  void bind_otherlib_dotnet_functions(dotnet_host& dn_host);

  namespace bindings {

    native_string native_get_program_files_folder(native_string app_name_str);
    native_string native_get_app_data_folder(native_string app_name_str, int32_t create_flag);
    native_string native_get_install_folder();

    bool native_begin_window(native_string title, int32_t flags);
    void native_end_window();
    bool native_begin_child(native_string str_id, ImVec2 size, bool border, int32_t flags);
    void native_end_child();

    void native_ui_text(native_string text);
    void native_ui_text_colored(float r, float g, float b, float a, native_string text);
    void native_ui_text_wrapped(native_string text);
    void native_ui_label_text(native_string label, native_string text);
    nbool32 native_ui_button(native_string label, float width, float height);
    nbool32 native_ui_small_button(native_string label);
    nbool32 native_ui_invisible_button(native_string str_id, float width, float height);
    nbool32 native_ui_checkbox(native_string label, nbool32* value);
    nbool32 native_ui_radio_button(native_string label, nbool32 active);
    void native_ui_progress_bar(float fraction, float width, float height, native_string overlay);
    void native_ui_separator();
    void native_ui_same_line(float offset, float spacing);
    void native_ui_spacing();
    void native_ui_indent(float width);
    void native_ui_unindent(float width);
    void native_ui_new_line();
    void native_ui_dummy(float width, float height);

    nbool32 native_ui_input_text(native_string label, native_string buffer, int32_t buffer_size, int32_t flags);
    nbool32 native_ui_input_text_multiline(native_string label, native_string buffer, int32_t buffer_size, float width, float height, int32_t flags);
    nbool32 native_ui_input_float(native_string label, float* value, float step, float step_fast, int32_t decimal_precision);
    nbool32 native_ui_input_float2(native_string label, float* values);
    nbool32 native_ui_input_float3(native_string label, float* values);
    nbool32 native_ui_input_float4(native_string label, float* values);
    nbool32 native_ui_input_int(native_string label, int32_t* value, int32_t step, int32_t step_fast);
    nbool32 native_ui_drag_float(native_string label, float* value, float speed, float min_val, float max_val);
    nbool32 native_ui_drag_float3(native_string label, float* values, float speed, float min_val, float max_val);
    nbool32 native_ui_slider_float(native_string label, float* value, float min_val, float max_val);
    nbool32 native_ui_slider_int(native_string label, int32_t* value, int32_t min_val, int32_t max_val);
    nbool32 native_ui_color_edit3(native_string label, float* col);
    nbool32 native_ui_color_edit4(native_string label, float* col);

    nbool32 native_ui_tree_node(native_string label);
    nbool32 native_ui_tree_node_ex(native_string label, int32_t flags);
    void native_ui_tree_pop();
    nbool32 native_ui_collapsing_header(native_string label, int32_t flags);

    nbool32 native_ui_selectable(native_string label, nbool32 selected, int32_t flags);
    nbool32 native_ui_begin_combo(native_string label, native_string preview_value, int32_t flags);
    void native_ui_end_combo();
    nbool32 native_ui_begin_listbox(native_string label, float width, float height);
    void native_ui_end_listbox();

    nbool32 native_ui_begin_tab_bar(native_string str_id, int32_t flags);
    void native_ui_end_tab_bar();
    nbool32 native_ui_begin_tab_item(native_string label, int32_t flags);
    void native_ui_end_tab_item();

    nbool32 native_ui_begin_menu_bar();
    void native_ui_end_menu_bar();
    nbool32 native_ui_begin_main_menu_bar();
    void native_ui_end_main_menu_bar();
    nbool32 native_ui_begin_menu(native_string label, nbool32 enabled);
    void native_ui_end_menu();
    nbool32 native_ui_menu_item(native_string label, native_string shortcut, nbool32 selected, nbool32 enabled);

    void native_ui_open_popup(native_string str_id);
    nbool32 native_ui_begin_popup(native_string str_id, int32_t flags);
    nbool32 native_ui_begin_popup_modal(native_string name, int32_t flags);
    void native_ui_end_popup();
    void native_ui_close_current_popup();

    nbool32 native_ui_begin_table(native_string str_id, int32_t columns, int32_t flags);
    void native_ui_end_table();
    void native_ui_table_next_row(int32_t flags, float min_row_height);
    nbool32 native_ui_table_next_column();
    nbool32 native_ui_table_set_column_index(int32_t column_n);
    void native_ui_table_setup_column(native_string label, int32_t flags, float init_width_or_weight);
    void native_ui_table_headers_row();

    void native_ui_get_content_region_avail(float* out_x, float* out_y);
    void native_ui_get_window_size(float* out_width, float* out_height);
    void native_ui_get_window_pos(float* out_x, float* out_y);
    void native_ui_set_next_window_size(float width, float height, int32_t cond);
    void native_ui_set_next_window_pos(float x, float y, int32_t cond);
    nbool32 native_ui_is_item_hovered();
    nbool32 native_ui_is_item_clicked(int32_t mouse_button);
    nbool32 native_ui_is_item_active();
    nbool32 native_ui_is_window_focused(int32_t flags);
    nbool32 native_ui_is_window_hovered(int32_t flags);

    void native_ui_push_style_color(int32_t idx, float r, float g, float b, float a);
    void native_ui_pop_style_color(int32_t count);
    void native_ui_push_style_var_float(int32_t idx, float val);
    void native_ui_push_style_var_vec2(int32_t idx, float x, float y);
    void native_ui_pop_style_var(int32_t count);
    void native_ui_push_item_width(float width);
    void native_ui_pop_item_width();

    void native_ui_push_id_str(native_string str_id);
    void native_ui_push_id_int(int32_t int_id);
    void native_ui_pop_id();

    void native_ui_set_tooltip(native_string text);
    nbool32 native_ui_begin_tooltip();
    void native_ui_end_tooltip();

    nbool32 native_ui_begin_drag_drop_source(int32_t flags);
    nbool32 native_ui_set_drag_drop_payload(native_string type, void* data, int32_t size);
    void native_ui_end_drag_drop_source();
    nbool32 native_ui_begin_drag_drop_target();
    void native_ui_end_drag_drop_target();

    void native_get_object_id(void* object_ptr, natural_t* out_id);
    natural_t native_scene_create_object(native_string name, float x, float y, float z);
    void native_scene_destroy_object(natural_t id);
    nbool32 native_scene_has_object(natural_t id);
    native_string native_scene_get_object_name(natural_t id);
    void native_scene_set_object_name(natural_t id, native_string name);
    void native_scene_get_object_ids(natural_t* out_ids, int32_t* out_count, int32_t max_count);
    natural_t native_scene_get_object_count();
    natural_t native_scene_find_object_by_name(native_string name);

    natural_t native_scene_get_parent_id(natural_t id);
    void native_scene_get_children_ids(natural_t id, natural_t* out_ids, int32_t* out_count, int32_t max_count);

    nbool32 native_scene_object_has_tag(natural_t id, native_string tag);
    void native_scene_add_object_tag(natural_t id, native_string tag);
    void native_scene_remove_object_tag(natural_t id, native_string tag);

    nbool32 native_scene_get_object_visible(natural_t id);
    void native_scene_set_object_visible(natural_t id, nbool32 visible);

    void native_transform_get_position(natural_t id, float* out_x, float* out_y, float* out_z);
    void native_transform_set_position(natural_t id, float x, float y, float z);
    void native_transform_get_rotation(natural_t id, float* out_x, float* out_y, float* out_z, float* out_w);
    void native_transform_set_rotation(natural_t id, float x, float y, float z, float w);
    void native_transform_get_scale(natural_t id, float* out_x, float* out_y, float* out_z);
    void native_transform_set_scale(natural_t id, float x, float y, float z);
    void native_transform_get_world_matrix(natural_t id, float* out_matrix);

    void native_component_add_by_name(integer_t id, native_string component_name);
    void native_component_remove_by_name(integer_t id, native_string component_name);
    nbool32 native_component_has_by_name(integer_t id, native_string component_name);

    void native_event_register(native_string event_name);
    void native_event_trigger(native_string event_name);
    void native_event_trigger_with_string(native_string event_name, native_string data);

    nbool32 native_input_is_key_down(int32_t sdl_scancode);
    nbool32 native_input_is_key_pressed(int32_t sdl_scancode);
    nbool32 native_input_is_mouse_button_down(int32_t button);
    nbool32 native_input_is_mouse_button_clicked(int32_t button);
    void native_input_get_mouse_position(float* out_x, float* out_y);
    void native_input_get_mouse_delta(float* out_x, float* out_y);
    float native_input_get_mouse_wheel();

    float native_time_get_delta_time();
    float native_time_get_elapsed_time();
    int64_t native_time_get_frame_count();

    int32_t native_driver_get_state();
    void native_driver_request_shutdown();
    native_string native_driver_get_project_name();

    native_string native_config_get_string(native_string section, native_string key, native_string default_value);
    int32_t native_config_get_int(native_string section, native_string key, int32_t default_value);
    float native_config_get_float(native_string section, native_string key, float default_value);
    nbool32 native_config_get_bool(native_string section, native_string key, nbool32 default_value);

    nbool32 native_network_is_connected();
    int32_t native_network_get_role();

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP