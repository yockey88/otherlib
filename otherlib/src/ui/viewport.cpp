/**
 * \file ui/viewport.cpp
 **/
#include "ui/viewport.hpp"

#include <imgui/imgui.h>

#include "renderer/render_pipeline.hpp"
#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_node.hpp"

#include "driver/driver.hpp"
#include "ui/asset-browser/asset_browser_widgets.hpp"  // For kDragDropPayloadType

namespace other {
  namespace ui {

    struct viewport_node : public ui_node {
      viewport_node(scope<renderer>& renderer_ptr, driver* driver_ptr, ui_window* parent, const std::string_view node_title)
          : ui_node(parent, node_title), renderer_ptr(renderer_ptr) {}
      virtual ~viewport_node() = default;

      scope<renderer>& renderer_ptr;

      void on_prepare_render() override {
      }

      void on_render_node_body() override {
        auto size = ImGui::GetContentRegionAvail();
        if (size.x != previous_size.x || size.y != previous_size.y) {
          events().trigger_event("viewport.resize", glm::vec2(size.x, size.y));
        }
        previous_size = size;

        const bool begin_scene_asset_drop = ImGui::BeginDragDropTarget();
        if (begin_scene_asset_drop) {
          if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(asset_browser_w::kDragDropPayloadType)) {
            // Handle the dropped scene asset here
            using data_t = asset_browser_w::asset_drag_drop_payload;
            data_t payload_data = *reinterpret_cast<const data_t*>(payload->Data);
            CORE_LOG_WARN("Dropped asset with ID: {} and type: {}", payload_data.handler_asset_id, payload_data.asset_type);
          }
          ImGui::EndDragDropTarget();
        }

        auto pipeline_outputs = renderer_ptr->get_pipeline_list();
        if (pipeline_outputs.empty()) {
          ImGui::Text("No rendering pipelines available.");
        } else {
          auto& pipeline = pipeline_outputs[0];
          ImTextureID tex_id = pipeline->get_final_output_texture_id();
          if (tex_id == 0) {
            scoped_color error_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
            ImGui::Text("No output texture available from the rendering pipeline.");
          } else {
            ImGui::Image(tex_id, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
          }
        }
      }

      void on_render_end() override {
      }

     private:
      ImVec2 previous_size = ImVec2(0, 0);
    };

    viewport::viewport(event_system& events, scope<renderer>& renderer_ptr, driver* driver_ptr)
        : ui_window(&events, "Viewport"), driver_ptr(driver_ptr) {
      events.register_event("viewport.resize");
      add_node(make_ref<viewport_node>(renderer_ptr, driver_ptr, this, "ViewportNode"));
    }

  }  // namespace ui
}  // namespace other