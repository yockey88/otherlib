/**
 * \file ui/viewport.cpp
 **/
#include "ui/viewport.hpp"

#include <imgui/imgui.h>

#include "renderer/render_pipeline.hpp"
#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_node.hpp"


namespace other {
  namespace ui {

    struct viewport_node : public ui_node {
      viewport_node(scope<renderer>& renderer_ptr, ui_window* parent, const std::string_view node_title)
          : ui_node(parent, node_title), renderer_ptr(renderer_ptr) {}
      virtual ~viewport_node() = default;

      scope<renderer>& renderer_ptr;

      void on_prepare_render() override {
      }

      void on_render_node_body() override {
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
            ImVec2 avail_size = ImGui::GetContentRegionAvail();
            ImGui::Image(tex_id, avail_size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
          }
        }
      }

      void on_render_end() override {
      }
    };

    viewport::viewport(event_system& events, scope<renderer>& renderer_ptr)
        : ui_window(events, "Viewport") {
      auto vp_node = make_scope<viewport_node>(renderer_ptr, this, "ViewportNode");
      add_node(std::move(vp_node));
    }

  }  // namespace ui
}  // namespace other