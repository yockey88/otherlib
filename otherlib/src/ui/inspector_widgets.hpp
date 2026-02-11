/**
 * \file ui/inspector_widgets.hpp
 **/
#ifndef OTHERLIB_UI_INSPECTOR_WIDGETS_HPP
#define OTHERLIB_UI_INSPECTOR_WIDGETS_HPP

#include <string>
#include <vector>

#include "core/defines.hpp"

#include "object/component.hpp"

#include "asset/asset.hpp"

namespace other {

  struct scene_object;

  namespace ui {
    namespace inspector {

      constexpr float kObjectHeaderHeight = 36.f;
      constexpr float kComponentHeaderHeight = 28.f;
      constexpr float kPropertyRowHeight = 24.f;
      constexpr float kPropertyLabelWidth = 200.f;
      constexpr float kPropertyLabelMinWidth = 110.f;
      constexpr float kFieldMaxWidth = 200.f;
      constexpr float kComponentDotRadius = 3.f;
      constexpr float kIconSize = 22.f;
      constexpr float kInnerPadding = 14.f;
      constexpr float kFieldHeight = 22.f;
      constexpr float kFieldGap = 3.f;
      constexpr float kAddComponentButtonHeight = 28.f;

      struct component_section_flags {
        bool modified = false;
        bool removable = true;
        bool disabled = false;
      };

      /// returns the glm::vec4 signature color for the given component type
      glm::vec4 get_component_color(component::id tag);

      // ═══════════════════════════════════════════════════════════════════════
      //  Object Header
      //
      //  ┌──────────────────────────────────────────┐
      //  │ [icon]  ObjectName              #0x4A2F  │
      //  └──────────────────────────────────────────┘
      // ═══════════════════════════════════════════════════════════════════════
      void draw_object_header(const std::string_view object_name, natural_t object_id, const glm::vec4& icon_color);
      /// editable version — returns true if name was changed, writes new
      /// name back into `name_buf` (must be at least `buf_size` chars)
      bool draw_object_header_editable(char* name_buf, uint32_t buf_size, natural_t object_id, const glm::vec4& icon_color);

      // ═══════════════════════════════════════════════════════════════════════
      //  Component Section
      //
      //  ┌──────────────────────────────────────────┐
      //  │ ● ComponentName                ● modified│
      //  ├──────────────────────────────────────────┤
      //  │  (property rows rendered by caller)      │
      //  └──────────────────────────────────────────┘
      //
      //  begin_component_section / end_component_section form a pair.
      //  begin returns false if the section is collapsed (caller should skip
      //  property rows but MUST still call end_component_section).
      // ═══════════════════════════════════════════════════════════════════════
      bool begin_component_section(const std::string_view component_name, component::id tag, component_section_flags flags = {}, bool* out_remove_requested = nullptr);
      void end_component_section();

      // ═══════════════════════════════════════════════════════════════════════
      //  Property Row
      //
      //  ┌──────────────────────────────────────────┐
      //  │ Label      [  field  ] [  field  ]       │
      //  └──────────────────────────────────────────┘
      //
      //  begin_property_row / end_property_row sets up the two-column
      //  layout.  The caller draws widgets in between for the value side.
      // ═══════════════════════════════════════════════════════════════════════
      void begin_property_row(const std::string_view label, float label_width = kPropertyLabelWidth);
      void end_property_row();

      bool drag_float_field(const char* id, float& value, float speed = 0.1f, const glm::vec4* axis_color = nullptr);
      bool drag_int8_field(const char* id, int8_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_uint8_field(const char* id, uint8_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_int16_field(const char* id, int16_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_uint16_field(const char* id, uint16_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_int32_field(const char* id, int32_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_uint32_field(const char* id, uint32_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_int64_field(const char* id, int64_t& value, const glm::vec4* axis_color = nullptr);
      bool drag_uint64_field(const char* id, uint64_t& value, const glm::vec4* axis_color = nullptr);
      bool input_text_field(const char* id, char* buf, uint32_t buf_size);
      void display_text_field(const char* id, const std::string_view text);
      bool inspector_checkbox(const char* id, bool& value);
      bool property_mat4(const std::string_view label, glm::mat4& value, float speed);
      void property_mat4_readonly(const std::string_view label, const glm::mat4& value);
      bool property_mat3(const std::string_view label, glm::mat3& value, float speed);
      void property_mat3_readonly(const std::string_view label, const glm::mat3& value);
      bool property_vec4(const std::string_view label, glm::vec4& value, float speed);
      void property_vec4_readonly(const std::string_view label, const glm::vec4& value);
      bool property_vec3(const std::string_view label, glm::vec3& value, float speed = 0.1f);
      void property_vec3_readonly(const std::string_view label, const glm::vec3& value);
      bool property_vec2(const std::string_view label, glm::vec2& value, float speed = 0.1f);
      void property_vec2_readonly(const std::string_view label, const glm::vec2& value);
      bool property_float(const std::string_view label, float& value, float speed = 0.1f);
      bool property_int8(const std::string_view label, int8_t& value);
      bool property_uint8(const std::string_view label, uint8_t& value);
      bool property_int16(const std::string_view label, int16_t& value);
      bool property_uint16(const std::string_view label, uint16_t& value);
      bool property_int32(const std::string_view label, int32_t& value);
      bool property_uint32(const std::string_view label, uint32_t& value);
      bool property_int64(const std::string_view label, int64_t& value);
      bool property_uint64(const std::string_view label, uint64_t& value);
      bool property_bool(const std::string_view label, bool& value);
      bool property_text(const std::string_view label, char* buf, uint32_t buf_size);
      void property_display(const std::string_view label, const std::string_view value_text, const glm::vec4& text_color = glm::vec4(0));

      //  asset reference field with colored border based on state.
      enum class asset_slot_state : uint32_t {
        empty = 0,
        filled,
        invalid,
        drag_hover,
      };

      void draw_asset_slot(const std::string_view label, const std::string_view asset_name, asset_slot_state state);

      /// returns id if an asset is dropped into the slot, or nullopt if no drop occurred
      opt<natural_t> property_asset_slot(const std::string_view label, natural_t asset_id, const std::string_view current_asset_name, const asset::type acceptable_type);

      bool draw_add_component_button();

      void draw_no_selection_message();
      void draw_multi_selection_message(uint32_t count);

    }  // namespace inspector
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_INSPECTOR_WIDGETS_HPP