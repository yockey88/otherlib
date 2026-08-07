/**
 * \file ui/asset-browser/asset_browser.cpp
 **/
#include "ui/asset-browser/asset_browser.hpp"

#include "core/profiler.hpp"
#include "theme/colors.hpp"

namespace other {
  namespace ui {

    asset_browser::asset_browser(event_system& events, driver* driver)
        : ui_window(&events, "Assets", true, ImGuiWindowFlags_None) {
      events.register_event("asset-browser.navigate");
      events.register_event("asset-browser.refresh");
      events.register_event("asset-browser.open-asset");

      tree_node = make_scope<asset_browser_tree_node>(this, driver);
      grid_node = make_scope<asset_browser_grid_node>(this, driver);

      /// wire tree directory click → grid navigation
      tree_node->set_navigate_callback([this](const std::string& path) {
        grid_node->navigate_to(path);
      });
    }

    void asset_browser::on_render_body() {
      PROFILE_SECTION("asset_browser::on_render_body");
      ImDrawList* dl = ImGui::GetWindowDrawList();
      ImVec2 avail = ImGui::GetContentRegionAvail();

      /// clamp splitter
      splitter_width = std::clamp(
        splitter_width, asset_browser_w::kDirTreeMinWidth,
        std::min(asset_browser_w::kDirTreeMaxWidth, avail.x * 0.5f));

      {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::asset_browser::kDirTreeBG));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginChild("##cb-tree-panel", ImVec2(splitter_width, avail.y), ImGuiChildFlags_None)) {
          tree_node->draw_content();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
      }

      ImGui::SameLine(0.f, 0.f);

      {
        constexpr float handle_w = 4.f;
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 handle_min = cursor;
        ImVec2 handle_max = { cursor.x + handle_w, cursor.y + avail.y };

        ImGui::SetCursorScreenPos(handle_min);

        if (avail.y == 0.f) {
          avail.y = 0.1f;
        }

        ImGui::InvisibleButton("##cb-splitter", ImVec2(handle_w, avail.y));
        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive();

        if (hovered || active) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (active) {
          splitter_width += ImGui::GetIO().MouseDelta.x;
        }

        ImU32 handle_col = {};
        if (active) {
          handle_col = colors::to_im_col(colors::kAccent);
        } else if (hovered) {
          handle_col = colors::to_im_col(colors::kBorderStrong);
        } else {
          handle_col = colors::to_im_col(colors::asset_browser::kBorder);
        }
        dl->AddRectFilled(handle_min, handle_max, handle_col);

        ImGui::SameLine(0.f, 0.f);
      }

      {
        float grid_w = avail.x - splitter_width - 4.f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::asset_browser::kBG));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginChild("##cb-grid-panel", ImVec2(grid_w, avail.y), ImGuiChildFlags_None)) {
          grid_node->draw_content();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
      }
    }

  }  // namespace ui
}  // namespace other