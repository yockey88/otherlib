/**
 * \file project_window.cpp
 **/
#include "project_window.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/ui_helpers.hpp"

namespace other {
  namespace ui {

    void project_window::on_render_header() {
      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("New Project")) {
            CORE_LOG_INFO("New Project menu item clicked");
          }
          if (ImGui::MenuItem("Open Project")) {
            CORE_LOG_INFO("Open Project menu item clicked");
          }
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }
    }

    void project_window::on_render_body() {
      scoped_color text_color{ ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextDisabled) };
      ImGui::Text("No Project loaded");
    }

  }  // namespace ui
}  // namespace other