/**
 * \file ui/asset_browser_tree_node.cpp
 **/
#include "ui/asset_browser_tree_node.hpp"

#include "core/subsystem.hpp"
#include "file/directory.hpp"
#include "file/filesystem.hpp"

#include "renderer/ui/colors.hpp"

#include "driver/driver.hpp"
#include "ui/asset_browser_widgets.hpp"

namespace other {
  namespace ui {

    namespace abw = asset_browser_w;

    namespace {

      void build_subtree(std::vector<abw::dir_tree_node>& nodes, const ref<directory>& dir, const std::string& base_path, int depth) {
        auto children = dir->child_directories();
        for (const auto& child : children) {
          std::string child_path = base_path + "/" + child->name();
          bool has_sub_dirs = !child->child_directories().empty();
          nodes.push_back({ child->name(), child_path, depth, false, has_sub_dirs });
          build_subtree(nodes, child, child_path, depth + 1);
        }
      }

    }  // namespace

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

      auto* fs = subsystem<file_system>::get();
      if (fs == nullptr) {
        return;
      }

      auto mount_names = fs->mounted_names();
      for (const auto& mount_name : mount_names) {
        auto mount = fs->get_mount(mount_name);
        if (mount == nullptr) {
          continue;
        }

        bool has_children = !mount->child_directories().empty();
        nodes.push_back({ mount_name, mount_name, 0, true, has_children });
        build_subtree(nodes, mount, mount_name, 1);
      }
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