/**
 * \file ui/node_editor.cpp
 **/
#include "ui/node_editor.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/logger.hpp"

namespace other {

  void node_editor::node::node_render_data::update_base_position(const glm::vec2& node_base_position, const glm::vec2& position, const glm::vec2& size) {
    full_node_size = size;
    global_node_pos = { node_base_position.x + position.x, node_base_position.y + position.y };
    full_node_max = global_node_pos + full_node_size;

    node_header_end = { full_node_max.x, global_node_pos.y + ImGui::GetFrameHeight() };
    node_body_begin = { global_node_pos.x, node_header_end.y };

    resize_triangle_p1 = { full_node_max.x - node_editor::kResizeHandleSize, full_node_max.y };
    resize_triangle_p2 = { full_node_max.x, full_node_max.y - node_editor::kResizeHandleSize };
    resize_triangle_p3 = { full_node_max.x, full_node_max.y };
  }

  void node_editor::node::draw_node(const glm::vec2& node_base_position) {
    if (ImGui::GetCurrentWindow()->SkipItems || state.error) {
      return;
    }

    ImGui::PushID(name.c_str());

    begin_node();
    if (state.error) {
      ImGui::PopID();
      state.error = false;
      return;
    }

    render_data.update_base_position(node_base_position, position, size);

    ImRect rect = ImRect{ { render_data.global_node_pos.x, render_data.global_node_pos.y }, { render_data.full_node_max.x, render_data.full_node_max.y } };
    ImRect titlebar_rect = ImRect{ rect.Min, { render_data.node_header_end.x, render_data.node_header_end.y } };
    ImRect body_rect = ImRect{ { render_data.node_body_begin.x, render_data.node_body_begin.y }, rect.Max };

    ImGui::GetWindowDrawList()->AddRectFilled(rect.Min, rect.Max, IM_COL32(60, 60, 60, 255), 4.0f);
    if (state.selected) {
      ImGui::GetWindowDrawList()->AddRectFilled(titlebar_rect.Min, titlebar_rect.Max, IM_COL32(255, 50, 50, 255));
      ImGui::GetWindowDrawList()->AddRectFilled(body_rect.Min, body_rect.Max, IM_COL32(50, 50, 255, 255));
    } else {
      ImGui::GetWindowDrawList()->AddRect(titlebar_rect.Min, titlebar_rect.Max, IM_COL32(255, 50, 50, 255));
      ImGui::GetWindowDrawList()->AddRect(body_rect.Min, body_rect.Max, IM_COL32(50, 50, 255, 255));
    }

    ImGui::GetWindowDrawList()->AddTriangleFilled(
      ImVec2{ render_data.resize_triangle_p1.x, render_data.resize_triangle_p1.y },
      ImVec2{ render_data.resize_triangle_p2.x, render_data.resize_triangle_p2.y },
      ImVec2{ render_data.resize_triangle_p3.x, render_data.resize_triangle_p3.y },
      ImGui::GetColorU32(state.stretching ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip)
    );

    ImGui::PopID();
  }

  void node_editor::node::begin_node() {
    ImGuiWindow* ig_window = ImGui::GetCurrentWindow();
    OTHER_ASSERT(ig_window != nullptr, "ImGui current window is null in node_editor::node::begin_node");
    OTHER_ASSERT(!ig_window->SkipItems, "ImGui current window is skipping items in node_editor::node::begin_node");

    ImRect rect{ { render_data.global_node_pos.x, render_data.global_node_pos.y }, { render_data.full_node_max.x, render_data.full_node_max.y } };
    ImRect titlebar_rect{ rect.Min, { render_data.node_header_end.x, render_data.node_header_end.y } };
    ImRect body_rect{ { render_data.node_body_begin.x, render_data.node_body_begin.y }, rect.Max };

    const ImGuiID id = ig_window->GetID(name.c_str());
    ImVec2 node_size = ImVec2(size.x, size.y);
    ImVec2 item_size = ImGui::CalcItemSize(node_size, 0.0f, 0.0f);

    ImGui::ItemSize(item_size);
    if (!ImGui::ItemAdd(rect, id, nullptr, ImGuiItemFlags_None)) {
      state.error = true;
      return;
    }

    bool previously_selected = state.selected;
    bool previously_stretching = state.stretching;
    bool previouslty_dragging = state.dragging;
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool mouse_in_titlebar = ImGui::IsMouseHoveringRect(titlebar_rect.Min, titlebar_rect.Max);

    state.hovered = ImGui::IsItemHovered();
    state.selected = (state.hovered || previously_selected) && mouse_down;

    const float kResizeHandleSize = 20.0f;
    const float kGripPadding = 4.0f;

    ImGuiMouseCursor cursor_to_use = ImGuiMouseCursor_Arrow;
    ImRect resize_grip_rect = {
      render_data.full_node_max.x - kResizeHandleSize - kGripPadding,
      render_data.full_node_max.y - kResizeHandleSize - kGripPadding,
      render_data.full_node_max.x + kGripPadding,
      render_data.full_node_max.y + kGripPadding,
    };

    bool hovering_boundary = ImGui::IsMouseHoveringRect(resize_grip_rect.Min, resize_grip_rect.Max);
    if (hovering_boundary) {
      if (ImGui::GetIO().MousePos.x > render_data.full_node_max.x - kResizeHandleSize) {
        cursor_to_use = ImGuiMouseCursor_ResizeEW;
      } else if (ImGui::GetIO().MousePos.y > render_data.full_node_max.y - kResizeHandleSize) {
        cursor_to_use = ImGuiMouseCursor_ResizeNS;
      }

      /// if EW || NS check if both
      if (cursor_to_use != ImGuiMouseCursor_Arrow) {
        if (ImGui::GetIO().MousePos.x > render_data.full_node_max.x - kResizeHandleSize &&
            ImGui::GetIO().MousePos.y > render_data.full_node_max.y - kResizeHandleSize) {
          cursor_to_use = ImGuiMouseCursor_ResizeNWSE;
        }
      }

      ImGui::SetMouseCursor(cursor_to_use);
      if (mouse_down) {
        state.stretching = true;
      }
    }

    state.stretching = (state.stretching || previously_stretching) && mouse_down && !previouslty_dragging;
    state.dragging = ((state.selected && mouse_in_titlebar) || (previouslty_dragging && mouse_down)) && !state.stretching;

    if (state.dragging) {
      position.x += ImGui::GetIO().MouseDelta.x;
      position.y += ImGui::GetIO().MouseDelta.y;

    } else if (state.stretching) {
      size.x += ImGui::GetIO().MouseDelta.x;
      size.y += ImGui::GetIO().MouseDelta.y;

      if (size.x < node_editor::node::kMinNodeDimension) {
        size.x = node_editor::node::kMinNodeDimension;
      }
      if (size.y < node_editor::node::kMinNodeDimension) {
        size.y = node_editor::node::kMinNodeDimension;
      }
    }
  }

}  // namespace other