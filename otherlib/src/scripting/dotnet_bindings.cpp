/**
 * \file scripting/dotnet_bindings.cpp
 **/
#include "scripting/dotnet_bindings.hpp"

#include <imgui/imgui.h>

#include "driver/driver.hpp"
#include "scripting/dotnet_bindings/component_bindings.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "scripting/dotnet_bindings/environment_api_bindings.hpp"
#include "scripting/dotnet_bindings/scene_bindings.hpp"
#include "scripting/dotnet_bindings/scene_object_bindings.hpp"
#include "scripting/dotnet_bindings/ui_bindings.hpp"

namespace other {
  namespace bindings {

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
    detail::set_dotnet_native_driver(drv);
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
      /// Scene
      .bind("SceneCreateObject", bindings::native_scene_create_object)
      .bind("SceneDestroyObject", bindings::native_scene_destroy_object)
      .bind("SceneHasObject", bindings::native_scene_has_object)
      .bind("SceneGetObjectName", bindings::native_scene_get_object_name)
      .bind("SceneSetObjectName", bindings::native_scene_set_object_name)
      .bind("SceneGetObjectIds", bindings::native_scene_get_object_ids)
      .bind("SceneGetObjectCount", bindings::native_scene_get_object_count)
      .bind("SceneFindObjectByName", bindings::native_scene_find_object_by_name)
      /// Object Hierarchy.
      .bind("SceneGetParentId", bindings::native_scene_get_parent_id)
      .bind("SceneGetChildrenIds", bindings::native_scene_get_children_ids)
      /// Object Tags.
      .bind("SceneObjectHasTag", bindings::native_scene_object_has_tag)
      .bind("SceneAddObjectTag", bindings::native_scene_add_object_tag)
      .bind("SceneRemoveObjectTag", bindings::native_scene_remove_object_tag)
      /// Object Visibility.
      .bind("SceneGetObjectVisible", bindings::native_scene_get_object_visible)
      .bind("SceneSetObjectVisible", bindings::native_scene_set_object_visible);

    bindings::binding_context{ dn_host }
      /// OtherObject
      .bind("GetObjectID", bindings::native_get_object_id)
      /// Components.
      .bind("ComponentAddByName", bindings::native_component_add_by_name)
      .bind("ComponentRemoveByName", bindings::native_component_remove_by_name)
      .bind("ComponentHasByName", bindings::native_component_has_by_name);

    // bindings::binding_context{ dn_host }
    //   /// SceneObject
    //   .bind("GetComponent", bindings::native_get_component);

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