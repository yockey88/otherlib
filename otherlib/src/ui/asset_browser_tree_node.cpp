/**
 * \file ui/asset_browser_tree_node.cpp
 **/
#include "ui/asset_browser_tree_node.hpp"

#include "renderer/ui/colors.hpp"

#include "ui/asset_browser_widgets.hpp"

namespace other {
  namespace ui {

    namespace abw = asset_browser_w;

    asset_browser_tree_node::asset_browser_tree_node(ui_window* window, driver* driver_ptr)
        : ui_node(window, "Asset Browser Tree"), driver_ptr(driver_ptr) {
      rebuild_tree();
    }

    void asset_browser_tree_node::set_navigate_callback(navigate_fn fn) {
      on_navigate = std::move(fn);
    }

    void asset_browser_tree_node::set_selected_path(const std::string& path) {
      selected_path = path;
    }

    void asset_browser_tree_node::refresh_tree() {
      rebuild_tree();
    }

    void asset_browser_tree_node::rebuild_tree() {
      nodes.clear();

      /// TODO(asset_registry): Replace with actual directory enumeration
      ///   e.g.  for (auto& dir : asset_registry::list_directories())
      ///
      /// Stub data for initial UI development:
      nodes.push_back({ "assets", "assets", 0, true, true });
      nodes.push_back({ "characters", "assets/characters", 1, true, true });
      nodes.push_back({ "player", "assets/characters/player", 2, false, true });
      nodes.push_back({ "enemy", "assets/characters/enemy", 2, false, true });
      nodes.push_back({ "npc", "assets/characters/npc", 2, false, true });
      nodes.push_back({ "environment", "assets/environment", 1, true, true });
      nodes.push_back({ "terrain", "assets/environment/terrain", 2, false, true });
      nodes.push_back({ "props", "assets/environment/props", 2, false, true });
      nodes.push_back({ "lighting", "assets/environment/lighting", 2, false, false });
      nodes.push_back({ "audio", "assets/audio", 1, false, true });
      nodes.push_back({ "scripts", "assets/scripts", 1, false, true });
      nodes.push_back({ "scenes", "assets/scenes", 1, false, true });
      nodes.push_back({ "ui", "assets/ui", 1, false, true });
    }

    void asset_browser_tree_node::on_render_node_body() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::asset_browser::kDirTreeBG));

      if (!ImGui::BeginChild("##dir-tree-scroll", ImVec2(0, 0), ImGuiChildFlags_None)) {
        ImGui::EndChild();
        ImGui::PopStyleColor();
        return;
      }

      /// iterate visible tree nodes (only show children of expanded parents)
      for (size_t i = 0; i < nodes.size(); ++i) {
        auto& node = nodes[i];

        /// skip children of collapsed parents
        if (node.depth > 0) {
          bool parent_expanded = true;
          for (int d = node.depth - 1; d >= 0; --d) {
            /// walk backwards to find the parent at this depth
            for (int j = static_cast<int>(i) - 1; j >= 0; --j) {
              if (nodes[j].depth == d) {
                if (!nodes[j].is_expanded) {
                  parent_expanded = false;
                }
                break;
              }
            }
            if (!parent_expanded) {
              break;
            }
          }
          if (!parent_expanded) {
            continue;
          }
        }

        bool is_selected = (node.path == selected_path);
        if (abw::draw_dir_tree_item(node, is_selected)) {
          selected_path = node.path;
          if (on_navigate) {
            on_navigate(node.path);
          }
        }
      }

      ImGui::EndChild();
      ImGui::PopStyleColor();
    }

  }  // namespace ui
}  // namespace other