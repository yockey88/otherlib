/**
 * \file ui/viewport/viewport_node.cpp
 **/
#include "ui/viewport/viewport_node.hpp"

#include "driver/driver.hpp"
#include "theme/colors.hpp"
#include "ui/asset-browser/asset_browser_widgets.hpp"  // For kDragDropPayloadType

namespace other {
  namespace ui {

    void viewport_node::on_prepare_render() {
    }

    void viewport_node::on_render_node_header() {
    }

    void viewport_node::on_render_node_body() {
      auto size = ImGui::GetContentRegionAvail();
      if (size.x != previous_size.x || size.y != previous_size.y) {
        events().trigger_event("viewport.resize", glm::vec2(size.x, size.y));
      }
      previous_size = size;

      const bool begin_scene_asset_drop = ImGui::BeginDragDropTarget();
      if (begin_scene_asset_drop) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(asset_browser_w::kDragDropPayloadType)) {
          using data_t = asset_browser_w::asset_drag_drop_payload;
          data_t payload_data = *reinterpret_cast<const data_t*>(payload->Data);
          CORE_LOG_WARN("Dropped asset with ID: {} and type: {}", payload_data.handler_asset_id, payload_data.asset_type);
        }
        ImGui::EndDragDropTarget();
      }

      auto pipeline_outputs = renderer_instance.get_pipeline_list();
      if (pipeline_outputs.empty()) {
        ImGui::Text("No rendering pipelines available.");
        return;
      }

      auto& pipeline = pipeline_outputs[0];
      ImTextureID tex_id = pipeline->get_final_output_texture_id();
      if (tex_id == 0) {
        scoped_color error_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
        ImGui::Text("No output texture available from the rendering pipeline.");
        return;
      }

      ImGui::Image(tex_id, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

      const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
      glm::vec2 click_pos = { 0, 0 };
      if (clicked) {
        ImVec2 local_viewport_mouse_pos = ImGui::GetMousePos();
        local_viewport_mouse_pos.x -= ImGui::GetItemRectMin().x;
        local_viewport_mouse_pos.y -= ImGui::GetItemRectMin().y;
        click_pos = { local_viewport_mouse_pos.x, local_viewport_mouse_pos.y };

        // then adjust so (0,0) is at the center of the viewport and y is flipped
        click_pos.x -= size.x / 2.0f;
        click_pos.y = size.y - click_pos.y - size.y / 2.0f;

        events().trigger_event("viewport.clicked", click_pos);
      }
    }

    void viewport_node::on_render_end() {
    }

  }  // namespace ui
}  // namespace other