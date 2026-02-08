/**
 * \file ui/asset_browser_grid_node.cpp
 **/
#include "ui/asset_browser_grid_node.hpp"

/**
 * \file ui/asset_browser_grid_node.cpp
 **/
#include <algorithm>
#include <sstream>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"

#include "renderer/ui/colors.hpp"

#include "ui/asset_browser_grid_node.hpp"
#include "ui/asset_browser_widgets.hpp"

namespace other {
  namespace ui {

    namespace cbw = asset_browser_w;

    asset_browser_grid_node::asset_browser_grid_node(ui_window* parent, driver* drvr)
        : ui_node(parent, "Content Browser Grid"), driver_ptr(drvr) {
      /// set up default filter pills
      filters = {
        { "All", cbw::asset_type::UNKNOWN, true },
        { "Texture", cbw::asset_type::TEXTURE, true },
        { "Model", cbw::asset_type::MODEL, true },
        { "Model Src", cbw::asset_type::MODEL_SOURCE, true },
        { "Animation", cbw::asset_type::ANIMATION, true },
        { "Script Src", cbw::asset_type::SCRIPT_SOURCE, true },
        { "Script", cbw::asset_type::SCRIPT, true },
        { "Audio", cbw::asset_type::AUDIO, true },
        { "Scene", cbw::asset_type::SCENE, true },
        { "Object", cbw::asset_type::SCENE_OBJECT, true },
      };

      navigate_to("assets");
    }

    void asset_browser_grid_node::navigate_to(const std::string& path) {
      current_path = path;
      selected_asset_idx = -1;
      rebuild_breadcrumbs();
      rebuild_asset_list();
    }

    void asset_browser_grid_node::refresh_listing() {
      rebuild_asset_list();
    }

    void asset_browser_grid_node::rebuild_breadcrumbs() {
      breadcrumbs.clear();
      std::istringstream stream(current_path);
      std::string segment;
      std::string accumulated;

      while (std::getline(stream, segment, '/')) {
        if (segment.empty()) {
          continue;
        }
        if (!accumulated.empty()) {
          accumulated += '/';
        }
        accumulated += segment;
        breadcrumbs.push_back({ segment, accumulated });
      }
    }

    void asset_browser_grid_node::rebuild_asset_list() {
      assets.clear();

      /// TODO(asset_registry): Replace with actual asset enumeration
      ///   e.g.  for (auto& entry : asset_registry::list(current_path_))
      ///
      /// Stub data for initial UI development:
      using at = cbw::asset_type;

      // assets.push_back({ "textures", "4 items", at::FOLDER });
      // assets.push_back({ "anims", "6 items", at::FOLDER });
      // assets.push_back({ "player_diffuse", "2048x2048 \xc2\xb7 4.2 MB", at::TEXTURE });
      // assets.push_back({ "player_normal", "2048x2048 \xc2\xb7 3.8 MB", at::TEXTURE });
      // assets.push_back({ "player_roughness", "1024x1024 \xc2\xb7 1.1 MB", at::TEXTURE });
      // assets.push_back({ "player_mesh.fbx", "12.4k tris \xc2\xb7 2.1 MB", at::MODEL_SOURCE });
      // assets.push_back({ "player_model", "compiled \xc2\xb7 890 KB", at::MODEL });
      // assets.push_back({ "idle", "2.4s \xc2\xb7 30fps", at::ANIMATION });
      // assets.push_back({ "run", "0.8s \xc2\xb7 30fps", at::ANIMATION });
      // assets.push_back({ "jump", "0.6s \xc2\xb7 30fps", at::ANIMATION });
      // assets.push_back({ "player_ctrl.lua", "142 lines \xc2\xb7 3.2 KB", at::SCRIPT_SOURCE });
      // assets.push_back({ "player_controller", "compiled \xc2\xb7 1.8 KB", at::SCRIPT });
      // assets.push_back({ "footstep_01", "0.3s \xc2\xb7 44.1kHz", at::AUDIO });
      // assets.push_back({ "jump_sfx", "0.5s \xc2\xb7 44.1kHz", at::AUDIO });
      // assets.push_back({ "player_prefab", "5 components", at::SCENE_OBJECT });
      // assets.push_back({ "test_arena", "47 objects", at::SCENE });
    }

    bool asset_browser_grid_node::passes_filter(const cbw::asset_card_desc& desc) const {
      /// folders always show
      if (desc.type == cbw::asset_type::FOLDER) {
        return true;
      }

      /// "All" pill (index 0) — if active, show everything
      if (!filters.empty() && filters[0].active) {
        return true;
      }

      /// check specific type filter
      for (const auto& f : filters) {
        if (f.type == desc.type && f.active) {
          return true;
        }
      }

      return false;
    }

    void asset_browser_grid_node::on_render_node_body() {
      using namespace colors;
      ImDrawList* dl = ImGui::GetWindowDrawList();

      {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;

        dl->AddRectFilled(cursor, { cursor.x + w, cursor.y + cbw::kToolbarHeight }, to_im_col(asset_browser::kBG));
        dl->AddLine(
          { cursor.x, cursor.y + cbw::kToolbarHeight },
          { cursor.x + w, cursor.y + cbw::kToolbarHeight },
          to_im_col(asset_browser::kBorder), 1.f
        );

        ImGui::SetCursorScreenPos({ cursor.x + cbw::kPaddingX, cursor.y + cbw::kPaddingY });

        int clicked_crumb = cbw::draw_breadcrumbs(breadcrumbs);
        if (clicked_crumb >= 0 && clicked_crumb < static_cast<int>(breadcrumbs.size())) {
          navigate_to(breadcrumbs[clicked_crumb].full_path);
        }

        /// search field on the right
        float search_w = 200.f;
        ImGui::SameLine(0.f, 0.f);
        float search_x = cursor.x + w - search_w - cbw::kPaddingX;
        ImGui::SetCursorScreenPos({ search_x, cursor.y + cbw::kPaddingY });
        if (cbw::draw_search_input(search_buf, sizeof(search_buf), search_w)) {
          search_lower = search_buf;
          std::ranges::transform(search_lower, search_lower.begin(), [](unsigned char c) { return std::tolower(c); });
        }

        ImGui::SetCursorScreenPos({ cursor.x, cursor.y + cbw::kToolbarHeight + 1.f });
      }

      {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;

        dl->AddRectFilled(cursor, { cursor.x + w, cursor.y + cbw::kFilterBarHeight }, to_im_col(colors::kBG0));
        dl->AddLine(
          { cursor.x, cursor.y + cbw::kFilterBarHeight },
          { cursor.x + w, cursor.y + cbw::kFilterBarHeight },
          to_im_col(asset_browser::kBorder), 1.f
        );

        ImGui::SetCursorScreenPos({ cursor.x + cbw::kPaddingX, cursor.y + 5.f });

        int visible_count = 0;
        for (const auto& a : assets) {
          if (passes_filter(a)) {
            ++visible_count;
          }
        }
        cbw::draw_filter_bar(filters, visible_count);

        ImGui::SetCursorScreenPos({ cursor.x, cursor.y + cbw::kFilterBarHeight + 1.f });
      }

      float card_w = cbw::card_width_from_zoom(zoom);
      float avail_w = ImGui::GetContentRegionAvail().x;
      float avail_h = ImGui::GetContentRegionAvail().y - cbw::kStatusBarHeight;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(asset_browser::kBG));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(cbw::kPaddingX, cbw::kPaddingX));

      if (ImGui::BeginChild("##asset-grid-scroll", ImVec2(0, avail_h), ImGuiChildFlags_None)) {
        int cols = std::max(1, static_cast<int>((avail_w - cbw::kPaddingX) / (card_w + cbw::kCardSpacing)));
        int col = 0;

        for (int i = 0; i < static_cast<int>(assets.size()); ++i) {
          auto& asset = assets[i];
          if (!passes_filter(asset)) {
            continue;
          }

          /// text search
          if (!search_lower.empty()) {
            std::string lower_name = asset.name;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), [](unsigned char c) { return std::tolower(c); });
            if (lower_name.find(search_lower) == std::string::npos) {
              continue;
            }
          }

          ImGui::PushID(i);

          if (col > 0) {
            ImGui::SameLine(0.f, cbw::kCardSpacing);
          }

          if (cbw::draw_asset_card(asset, card_w)) {
            /// deselect all others
            for (int j = 0; j < static_cast<int>(assets.size()); ++j) {
              assets[j].is_selected = (j == i);
            }
            selected_asset_idx = i;

            /// double-click to open
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
              if (asset.type == cbw::asset_type::FOLDER) {
                navigate_to(current_path + "/" + asset.name);
              }
              /// TODO(asset_browser): fire open-asset event for non-folder types
            }
          }

          ImGui::PopID();

          col++;
          if (col >= cols) {
            col = 0;
          }
        }
      }
      ImGui::EndChild();

      ImGui::PopStyleVar();
      ImGui::PopStyleColor();

      {
        int asset_count = 0;
        int folder_count = 0;
        for (const auto& a : assets) {
          if (!passes_filter(a)) {
            continue;
          }
          if (a.type == cbw::asset_type::FOLDER) {
            folder_count++;
          } else {
            asset_count++;
          }
        }

        cbw::status_bar_info info;
        info.total_assets = asset_count;
        info.folder_count = folder_count;
        info.selected_name = (selected_asset_idx >= 0 && selected_asset_idx < static_cast<int>(assets.size())) ? assets[selected_asset_idx].name.c_str() : nullptr;
        info.zoom_normalized = zoom;

        if (cbw::draw_status_bar(info)) {
          zoom = info.zoom_normalized;
        }
      }
    }

  }  // namespace ui
}  // namespace other