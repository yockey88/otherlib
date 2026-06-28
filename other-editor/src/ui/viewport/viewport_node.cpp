/**
 * \file ui/viewport/viewport_node.cpp
 **/
#include "ui/viewport/viewport_node.hpp"

#include <glm/gtc/type_ptr.hpp>

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

      scene_object* selected_object = nullptr;
      opt<glm::mat4> selected_local_to_world_matrix = std::nullopt;
      opt<glm::mat4> selected_transform = std::nullopt;
      if (editor_ctx.active_scene != nullptr && !editor_ctx.current_selection.objects.empty()) {
        selected_object = editor_ctx.active_scene->find_object(editor_ctx.current_selection.objects[0]);
        if (selected_object != nullptr) {
          transform& t = editor_ctx.active_scene->get_transform(selected_object);
          selected_transform = t.get_local_model_matrix();
          selected_local_to_world_matrix = editor_ctx.active_scene->get_local_to_world_matrix(selected_object);
        }
      }

      ImTextureID tex_id = renderer_instance.get_debug_overlay_id("default-instancing");
      if (tex_id == 0) {
        scoped_color error_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
        ImGui::Text("Requested viewport texture %s does not exist.", std::format("{}", "default-instancing").c_str());
        tex_id = renderer_instance.get_texture_id("default-instancing", "smaa_texture");
        if (tex_id == 0) {
          ImGui::Text("No fallback texture available.");
          return;
        }
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

      if (selected_transform.has_value()) {
        OTHER_ASSERT(selected_object != nullptr, "If selected_transform has a value, selected_object must not be null.");
        OTHER_ASSERT(selected_local_to_world_matrix.has_value(), "If selected_transform has a value, selected_local_to_world_matrix must also have a value.");
        glm::mat4 world_transform = selected_local_to_world_matrix.value() * selected_transform.value();

        glm::ivec2 win_size = { size.x, size.y };
        glm::mat4 camera_view = editor_ctx.editor_camera.get_view_matrix();
        glm::mat4 camera_proj = editor_ctx.editor_camera.get_projection_matrix(win_size);

        const bool manip = ImGuizmo::Manipulate(glm::value_ptr(camera_view), glm::value_ptr(camera_proj), editor_ctx.gizmo_operation, editor_ctx.gizmo_mode, glm::value_ptr(world_transform), nullptr, nullptr);
        if (manip) {
          glm::mat4 local_manip = glm::inverse(selected_local_to_world_matrix.value()) * world_transform;

          float pos_mat[3], rot_mat[3], scale_mat[3];
          ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(local_manip), pos_mat, rot_mat, scale_mat);

          transform t;
          t.local_position = glm::vec3(pos_mat[0], pos_mat[1], pos_mat[2]);
          t.set_local_rotation(glm::vec3(rot_mat[0], rot_mat[1], rot_mat[2]));
          t.local_scale = glm::vec3(scale_mat[0], scale_mat[1], scale_mat[2]);

          editor_ctx.active_scene->set_transform(selected_object, t);
        }
      }
    }

    void viewport_node::on_render_end() {
    }

  }  // namespace ui
}  // namespace other