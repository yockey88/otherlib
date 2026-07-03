/**
 * \file ui/viewport/viewport.cpp
 **/
#include "ui/viewport/viewport.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "renderer/render_pipeline.hpp"

#include "theme/colors.hpp"
#include "ui/asset-browser/asset_browser_widgets.hpp"
#include "ui/ui_node.hpp"

#include "editor_driver.hpp"

namespace other {
  namespace ui {

    viewport::viewport(editor_context& ctx, event_system& events, renderer& renderer_instance, driver* driver_ptr)
        : ui_window(&events, "Viewport", true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar), renderer_instance(renderer_instance), driver_ptr(driver_ptr), editor_ctx(ctx) {
    }

    void viewport::custom_render() {
      auto viewports = get_driver().get_kernel().get_core_system<rendering_system>().get_viewports();
      for (auto& vp : viewports) {
        draw_viewport(vp);
      }
    }

    // void viewport::on_render_header() {
    //   ui::menu options_menu = {
    //     "Options"
    //   };
    //   ui::menu view_menu = {
    //     "View"
    //   };
    //   ui::menu tools_menu = {
    //     "Tools"
    //   };
    //   ui::menu help_menu = {
    //     "Help"
    //   };
    // }

    void viewport::draw_viewport(other::viewport& vp) {
      auto& kernel = get_driver().get_kernel();
      auto& rendering_sys = kernel.get_core_system<rendering_system>();

      const bool begin = ImGui::Begin(vp.name.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
      if (!begin) {
        ImGui::End();
        return;
      }

      // const bool begin_scene_asset_drop = ImGui::BeginDragDropTarget();
      // if (begin_scene_asset_drop) {
      //   if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(asset_browser_w::kDragDropPayloadType)) {
      //     using data_t = asset_browser_w::asset_drag_drop_payload;
      //     data_t payload_data = *reinterpret_cast<const data_t*>(payload->Data);
      //     CORE_LOG_WARN("Dropped asset with ID: {} and type: {}", payload_data.handler_asset_id, payload_data.asset_type);
      //   }
      //   ImGui::EndDragDropTarget();
      // }

      auto size = ImGui::GetContentRegionAvail();
      vp.size = glm::ivec2(size.x, size.y);

      ImTextureID tex_id = rendering_sys.get_texture_id(vp.texture);
      if (tex_id == 0) {
        scoped_color error_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
        ImGui::Text("Requested viewport texture %s does not exist.", vp.name.c_str());
        tex_id = renderer_instance.get_texture_id("default-instancing", "smaa_texture");
        if (tex_id == 0) {
          ImGui::Text("No fallback texture available.");
          return;
        }
      }

      if (vp.cam == nullptr) {
        scoped_color error_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
        ImGui::Text("Viewport '%s' has no camera assigned.", vp.name.c_str());
      } else {
        ImGui::Image(tex_id, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        // const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        // glm::vec2 click_pos = { 0, 0 };
        // if (clicked) {
        //   ImVec2 local_viewport_mouse_pos = ImGui::GetMousePos();
        //   local_viewport_mouse_pos.x -= ImGui::GetItemRectMin().x;
        //   local_viewport_mouse_pos.y -= ImGui::GetItemRectMin().y;
        //   click_pos = { local_viewport_mouse_pos.x, local_viewport_mouse_pos.y };

        //   // then adjust so (0,0) is at the center of the viewport and y is flipped
        //   click_pos.x -= size.x / 2.0f;
        //   click_pos.y = size.y - click_pos.y - size.y / 2.0f;

        //   // has to go to the right place
        //   // get_event_system().trigger_event("viewport.clicked", click_pos);
        // }
      }

      ImGui::End();
    }

  }  // namespace ui
}  // namespace other