/**
 * \file scripting/bindings.cpp
 **/
#include "scripting/bindings.hpp"

#include "core/logger.hpp"

#include "script/scripting_environment.hpp"

#include "object/animation_component.hpp"
#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/grid_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"

#include "driver/driver.hpp"
#include "scripting/binding_descriptor.hpp"
#include "scripting/dotnet_bindings/component_bindings.hpp"
#include "scripting/dotnet_bindings/core_bindings.hpp"
#include "scripting/dotnet_bindings/draw_bindings.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"
#include "scripting/dotnet_bindings/environment_api_bindings.hpp"
#include "scripting/dotnet_bindings/event_bindings.hpp"
#include "scripting/dotnet_bindings/input_bindings.hpp"
#include "scripting/dotnet_bindings/network_bindings.hpp"
#include "scripting/dotnet_bindings/scene_bindings.hpp"
#include "scripting/dotnet_bindings/scene_object_bindings.hpp"
#include "scripting/dotnet_bindings/ui_bindings.hpp"
#include "scripting/lua_bindings/bind_math_types_lua.hpp"
#include "scripting/lua_bindings/bind_rendering_types_lua.hpp"
#include "scripting/other_abi.hpp"
#include "scripting/scene_interface.hpp"
#include "tools/environment_console.hpp"

namespace other {
  namespace detail {

    void bind_native_types_lua(sol::state& lua_state);
    void bind_native_types_dotnet(dotnet_host& dn_host);
    void bind_scene_interface(driver* drv);
    void bind_abi_functions(dotnet_host& dn_host);

  }  // namespace detail

  namespace bindings {

    template <typename Fn>
    void bind_function(dotnet_host& dn_host, native_string name, Fn fn) {
      PROFILE_SECTION("other::bindings::bind-function");
      void* fn_ptr = (void*)fn;
      dn_host.interop().bind_native_function(name, fn_ptr);
    }

    void validate_binding_points(dotnet_host& dn_host) {
      PROFILE_SECTION("other::bindings::validate-binding-points");
      CORE_LOG_DEBUG("Validating native function binding points...");

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

  void bind_otherlib_dotnet_functions(dotnet_host& dn_host) {
    PROFILE_SECTION("other::bind-otherlib-dotnet-functions");
    dn_host.rediscover_binding_points();

    detail::bind_native_types_dotnet(dn_host);
    detail::bind_abi_functions(dn_host);

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
      /// Dynamic drawing (in-scene overlay or debug view).
      .bind("DrawLine", bindings::native_draw_line)
      .bind("DrawTriangle", bindings::native_draw_triangle)
      .bind("DrawPoint", bindings::native_draw_point)
      .bind("DrawRay", bindings::native_draw_ray)
      .bind("DrawArrow", bindings::native_draw_arrow)
      .bind("DrawAABB", bindings::native_draw_aabb)
      .bind("DrawOBB", bindings::native_draw_obb)
      .bind("DrawSphere", bindings::native_draw_sphere)
      .bind("DrawFrustum", bindings::native_draw_frustum)
      .bind("DrawTransform", bindings::native_draw_transform)
      .bind("DrawMesh", bindings::native_draw_mesh)
      .bind("DrawGrid", bindings::native_draw_grid);

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

    bindings::binding_context{ dn_host }
      /// OtherObject
      .bind("ValidateObjectHandle", bindings::native_validate_object_handle)
      .bind("GetObjectID", bindings::native_get_object_id);

    bindings::binding_context{ dn_host }
      /// Scene Object
      .bind("GetWorldMatrix", bindings::native_transform_get_world_matrix);

    bindings::binding_context{ dn_host }
      /// Component Helpers
      .bind("GetNumVertices", bindings::native_render_component_get_num_vertices)
      .bind("GetNumIndices", bindings::native_render_component_get_num_indices)
      .bind("GetMeshName", bindings::native_render_component_get_mesh_name)
      .bind("FetchMesh", bindings::native_render_component_fetch_mesh)
      .bind("UploadMesh", bindings::native_render_component_upload_mesh)
      .bind("GetMaterialPath", bindings::native_render_component_get_material_path)
      .bind("SetMaterialPath", bindings::native_render_component_set_material_path)
      .bind("GetAnimationClipPath", bindings::native_animation_component_get_clip_path)
      .bind("SetAnimationClipPath", bindings::native_animation_component_set_clip_path)
      .bind("GetAudioClipPath", bindings::native_audio_source_get_clip_path)
      .bind("SetAudioClipPath", bindings::native_audio_source_set_clip_path)
      .bind("GetPhysicsBodyType", bindings::native_physics_component_get_body_type)
      .bind("SetPhysicsBodyType", bindings::native_physics_component_set_body_type)
      .bind("GetPhysicsMass", bindings::native_physics_component_get_mass)
      .bind("SetPhysicsMass", bindings::native_physics_component_set_mass)
      .bind("GetPhysicsIsTrigger", bindings::native_physics_component_get_is_trigger)
      .bind("SetPhysicsIsTrigger", bindings::native_physics_component_set_is_trigger)
      .bind("GetPhysicsLinearVelocity", bindings::native_physics_component_get_linear_velocity)
      .bind("SetPhysicsLinearVelocity", bindings::native_physics_component_set_linear_velocity)
      .bind("GetPhysicsAngularVelocity", bindings::native_physics_component_get_angular_velocity)
      .bind("SetPhysicsAngularVelocity", bindings::native_physics_component_set_angular_velocity)
      .bind("PhysicsAddForce", bindings::native_physics_component_add_force)
      .bind("PhysicsAddImpulse", bindings::native_physics_component_add_impulse)
      .bind("PhysicsAddTorque", bindings::native_physics_component_add_torque)
      .bind("PhysicsRaycast", bindings::native_physics_raycast)
      .bind("CameraGetPosition", bindings::native_camera_component_get_position)
      .bind("CameraLookFrom", bindings::native_camera_component_look_from)
      .bind("CameraGetDirection", bindings::native_camera_component_get_direction)
      .bind("CameraLookAt", bindings::native_camera_component_look_at)
      .bind("CameraLook", bindings::native_camera_component_look)
      .bind("CameraGetFov", bindings::native_camera_component_get_fov)
      .bind("CameraSetFov", bindings::native_camera_component_set_fov)
      .bind("CameraGetClipPlanes", bindings::native_camera_component_get_clip_planes)
      .bind("CameraSetClipPlanes", bindings::native_camera_component_set_clip_planes)
      .bind("PointLightGetPosition", bindings::native_point_light_get_position)
      .bind("PointLightSetPosition", bindings::native_point_light_set_position)
      .bind("PointLightGetColor", bindings::native_point_light_get_color)
      .bind("PointLightSetColor", bindings::native_point_light_set_color)
      .bind("DirectionLightGetDirection", bindings::native_direction_light_get_direction)
      .bind("DirectionLightSetDirection", bindings::native_direction_light_set_direction)
      .bind("DirectionLightGetColor", bindings::native_direction_light_get_color)
      .bind("DirectionLightSetColor", bindings::native_direction_light_set_color);

    bindings::binding_context{ dn_host }
      /// Audio
      .bind("AudioPlayOneShot", bindings::native_audio_play_one_shot)
      .bind("AudioSetBusVolume", bindings::native_audio_set_bus_volume)
      .bind("AudioGetBusVolume", bindings::native_audio_get_bus_volume);

    bindings::validate_binding_points(dn_host);
  }

  void bind_otherlib_lua_functions(lua_host& lua_host) {
    PROFILE_SECTION("bind_otherlib_lua_functions");
    filepath lua_defs_path = lua_host.retrieve_script_path("global_definitions.lua");
    filepath lua_bridge_path = lua_host.retrieve_script_path("other_bridge.lua");
    if (!std::filesystem::exists(lua_defs_path)) {
      CORE_LOG_ERROR("Lua global definitions script '{}' does not exist.", lua_defs_path.string());
      return;
    }

    if (!std::filesystem::exists(lua_bridge_path)) {
      CORE_LOG_ERROR("Lua bridge script '{}' does not exist.", lua_bridge_path.string());
      return;
    }

    sol::state& lua_state = lua_host.get_lua_state();
    detail::bind_native_types_lua(lua_state);

    lua_state.create_named_table(
      "__other_native",
      "__log", lua_state.create_table_with(),
      "__driver", lua_state.create_table_with(),
      "__environment_console", lua_state.create_table_with(),
      "__native_scene", lua_state.create_table_with(),
      "__scene_interface", lua_state.create_table_with(),
      "__component_names", lua_state.create_table_with(),
      "__dotnet_types", lua_state.create_table_with());

    lua_state.create_named_table(
      "__lua_bridge_metadata",
      "__paths", lua_state.create_table_with());

    sol::table paths_table = lua_state["__lua_bridge_metadata"]["__paths"];
    paths_table["script_directory"] = lua_host.get_environment_script_directory().string();
    paths_table["global_definitions"] = lua_defs_path.string();
    paths_table["other_bridge"] = lua_bridge_path.string();

    {
      PROFILE_SECTION("bind_otherlib_lua_functions--run-global-definitions");
      CORE_LOG_DEBUG("Loading lua global definitions script '{}'.", lua_defs_path.string());
      lua_state.script_file(lua_defs_path.string());
    }

    sol::table log_table = lua_state["__other_native"]["__log"];
    log_table.set_function("send_log_message", [](spdlog::level::level_enum level, const std::string& message, const std::string& source, int line) {
      other::subsystem<other::logger>::get()->send_log(level, 0, std::format(" [Lua] {} @ ({}:{})", message, source, line));
    });

    {
      PROFILE_SECTION("bind_otherlib_lua_functions--run-bridge-script");
      CORE_LOG_DEBUG("Loading lua bridge script '{}'.", lua_bridge_path.string());
      lua_state.script_file(lua_bridge_path.string());
    }
  }

  void do_script_interface_bindings(driver* drv) {
    OTHER_ASSERT(drv != nullptr, "Driver pointer is null in do_script_interface_bindings.");

    detail::set_dotnet_native_driver(drv);
    abi::oe_init_abi(drv);
  }

  void do_script_interface_unbinding() {
    abi::oe_cleanup_abi();
  }

  namespace detail {
    template <typename T>
      requires reflected_type<T>
    std::string get_native_type_name() {
      std::string full_name = std::string{ refl::reflect<T>().name };
      size_t last_scope = full_name.rfind("::");
      if (last_scope != std::string::npos) {
        return full_name.substr(last_scope + 2);
      }
      return full_name;
    }

    /// \todo this @p lua_name feels fragile although changing it in the lua code on accident would be immediately obvious so maybe it's fine?
    template <typename T>
      requires reflected_type<T>
    void bind_lua_component(sol::state& lua_state, const std::string_view lua_name) {
      CORE_LOG_DEBUG(" - binding to Lua: '{}'", get_native_type_name<T>());
      sol::usertype<T> lua_usertype = lua_state.new_usertype<T>(lua_name);
      refl::util::for_each(refl::reflect<T>().members, [&](auto member) {
        if constexpr (!refl::descriptor::is_function(member) && refl::descriptor::has_attribute<attr::serializable>(member)) {
          using field_t = std::remove_cvref_t<decltype(member(std::declval<T&>()))>;
          field_t T::* field_ptr = member.pointer;
          std::string name = std::string{ member.name };
          lua_usertype.set(name, field_ptr);
          CORE_LOG_TRACE("   - Bound field '{}' of type [{}]", name, typeid(field_t).name());
        }
      });

      /// ensures bound types are registered in the type database
      reflection_data& _ = type_data_handler<T>::get_reflection_data(T{});
    }

    template <typename T>
      requires reflected_type<T>
    void bind_dotnet_component(dotnet_host& dn_host, dotnet_object* managed_binder) {
      // CORE_LOG_DEBUG(" - binding to .NET: '{}'", get_native_type_name<T>());

      auto desc = make_component_descriptor<T>(get_native_type_name<T>());
      set_ecs_lifecycle<T>(desc);
      abi::oe_get_registry().register_component(std::move(desc));
    }

    void bind_native_types_lua(sol::state& lua_state) {
      PROFILE_SECTION("bind_native_types_lua");
      lua_state.new_enum(
        "log_level",
        "TRACE", spdlog::level::trace,
        "DEBUG", spdlog::level::debug,
        "INFO", spdlog::level::info,
        "WARN", spdlog::level::warn,
        "ERROR", spdlog::level::err,
        "CRITICAL", spdlog::level::critical);
      lua_state.new_enum(
        "console_message",
        "CONSOLE_NONE", CONSOLE_MESSAGE_NONE,
        "CONSOLE_MESSAGE", CONSOLE_MESSAGE_MESSAGE,
        "CONSOLE_TRACE", CONSOLE_MESSAGE_TRACE,
        "CONSOLE_DEBUG", CONSOLE_MESSAGE_DEBUG,
        "CONSOLE_INFO", CONSOLE_MESSAGE_INFO,
        "CONSOLE_WARN", CONSOLE_MESSAGE_WARN,
        "CONSOLE_ERROR", CONSOLE_MESSAGE_ERROR,
        "CONSOLE_COMMAND", CONSOLE_MESSAGE_COMMAND);
      lua_state.new_enum(
        "driver_state",
        "STOPPED", driver_state::DRIVER_STATE_STOPPED,
        "INITIALIZING", driver_state::DRIVER_STATE_INITIALIZING,
        "RUNNING", driver_state::DRIVER_STATE_RUNNING,
        "PAUSED", driver_state::DRIVER_STATE_PAUSED,
        "SHUTTING_DOWN", driver_state::DRIVER_STATE_SHUTTING_DOWN,
        "NUM_DRIVER_STATES", driver_state::NUM_STATES);
      lua_state.new_enum(
        "driver_event",
        "START", driver_event::DRIVER_EVENT_START,
        "READY", driver_event::DRIVER_EVENT_READY,
        "PAUSE", driver_event::DRIVER_EVENT_PAUSE,
        "RESUME", driver_event::DRIVER_EVENT_RESUME,
        "STOP", driver_event::DRIVER_EVENT_STOP,
        "NUM_DRIVER_EVENTS", driver_event::NUM_EVENTS);

      bind_linear_algebra_types(lua_state);
      bind_rendering_types(lua_state);

      /// bind scene components (and other native types)
      CORE_LOG_DEBUG("Binding native scene components to Lua");
      bind_lua_component<scene_object>(lua_state, "__native_scene_object");
      bind_lua_component<transform>(lua_state, "__native_transform_component");
      bind_lua_component<script_component>(lua_state, "__native_script_component");
      bind_lua_component<render_component>(lua_state, "__native_render_component");
      bind_lua_component<camera_component>(lua_state, "__native_camera_component");
      bind_lua_component<grid_component>(lua_state, "__native_grid_component");
      bind_lua_component<point_light_component>(lua_state, "__native_point_light_component");
      bind_lua_component<direction_light_component>(lua_state, "__native_direction_light_component");
      bind_lua_component<animation_component>(lua_state, "__native_animation_component");
      bind_lua_component<audio_source_component>(lua_state, "__native_audio_source_component");
      bind_lua_component<audio_listener_component>(lua_state, "__native_audio_listener_component");
    }

    void bind_native_types_dotnet(dotnet_host& dn_host) {
      PROFILE_SECTION("bind_native_types_dotnet");
      CORE_LOG_DEBUG("Binding native types to .NET");

      dotnet_object* obj = nullptr;
      // dn_host.instantiate_managed_object("Other.TypeBinder", "Binder");

      bind_dotnet_component<transform>(dn_host, obj);
      bind_dotnet_component<script_component>(dn_host, obj);
      bind_dotnet_component<render_component>(dn_host, obj);
      bind_dotnet_component<camera_component>(dn_host, obj);
      bind_dotnet_component<grid_component>(dn_host, obj);
      bind_dotnet_component<point_light_component>(dn_host, obj);
      bind_dotnet_component<direction_light_component>(dn_host, obj);
      bind_dotnet_component<animation_component>(dn_host, obj);
      bind_dotnet_component<audio_source_component>(dn_host, obj);
      bind_dotnet_component<audio_listener_component>(dn_host, obj);
      bind_dotnet_component<physics_component>(dn_host, obj);

      // dn_host.destroy_managed_object(obj);
    }

    void bind_scene_interface(driver* drv) {
      OTHER_ASSERT(drv != nullptr, "Driver pointer is null in bind_scene_interface.");
    }

    void bind_abi_functions(dotnet_host& dn_host) {
      PROFILE_SECTION("other::bind-abi-functions");

      bindings::binding_context{ dn_host }
        /// Core.
        .bind("OeFnvHash", bindings::native_oe_fnv_hash);

      bindings::binding_context{ dn_host }
        /// Validation.
        .bind("OeValidateHandle", abi::oe_validate_handle)
        /// Component management.
        .bind("OeHasComponent", abi::oe_has_component)
        .bind("OeAddComponent", abi::oe_add_component)
        .bind("OeRemoveComponent", abi::oe_remove_component);

      bindings::binding_context{ dn_host }
        /// Field accessors - Primitives.
        .bind("OeGetFieldBool", abi::oe_get_field_bool)
        .bind("OeSetFieldBool", abi::oe_set_field_bool)
        .bind("OeGetFieldI32", abi::oe_get_field_i32)
        .bind("OeSetFieldI32", abi::oe_set_field_i32)
        .bind("OeGetFieldU32", abi::oe_get_field_u32)
        .bind("OeSetFieldU32", abi::oe_set_field_u32)
        .bind("OeGetFieldI64", abi::oe_get_field_i64)
        .bind("OeSetFieldI64", abi::oe_set_field_i64)
        .bind("OeGetFieldU64", abi::oe_get_field_u64)
        .bind("OeSetFieldU64", abi::oe_set_field_u64)
        .bind("OeGetFieldF32", abi::oe_get_field_f32)
        .bind("OeSetFieldF32", abi::oe_set_field_f32)
        .bind("OeGetFieldF64", abi::oe_get_field_f64)
        .bind("OeSetFieldF64", abi::oe_set_field_f64);

      bindings::binding_context{ dn_host }
        /// Field accessors - Math types.
        .bind("OeGetFieldVec2", abi::oe_get_field_vec2)
        .bind("OeSetFieldVec2", abi::oe_set_field_vec2)
        .bind("OeGetFieldVec3", abi::oe_get_field_vec3)
        .bind("OeSetFieldVec3", abi::oe_set_field_vec3)
        .bind("OeGetFieldVec4", abi::oe_get_field_vec4)
        .bind("OeSetFieldVec4", abi::oe_set_field_vec4)
        .bind("OeGetFieldQuat", abi::oe_get_field_quat)
        .bind("OeSetFieldQuat", abi::oe_set_field_quat)
        .bind("OeGetFieldMat4", abi::oe_get_field_mat4)
        .bind("OeSetFieldMat4", abi::oe_set_field_mat4);

      bindings::binding_context{ dn_host }
        /// Field accessors - String.
        .bind("OeGetFieldString", abi::oe_get_field_string)
        .bind("OeSetFieldString", abi::oe_set_field_string);
    }

  }  // namespace detail
}  // namespace other