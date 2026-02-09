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
#include "core/fnv.hpp"
#include "core/subsystem.hpp"
#include "file/directory.hpp"
#include "file/file_handle.hpp"
#include "file/filesystem.hpp"

#include "renderer/ui/colors.hpp"

#include "driver/driver.hpp"
#include "ui/asset_browser_grid_node.hpp"
#include "ui/asset_browser_widgets.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"


namespace other {
  namespace ui {

    namespace cbw = asset_browser_w;

    namespace {

      cbw::asset_type map_asset_to_ui_type(asset::type t) {
        switch (t) {
          case asset::TEXTURE: return cbw::asset_type::TEXTURE;
          case asset::MODEL_SOURCE: return cbw::asset_type::MODEL_SOURCE;
          case asset::MODEL: return cbw::asset_type::MODEL;
          case asset::ANIMATION: return cbw::asset_type::ANIMATION;
          case asset::SCRIPT_SOURCE: return cbw::asset_type::SCRIPT_SOURCE;
          case asset::SCRIPT: return cbw::asset_type::SCRIPT;
          case asset::AUDIO: return cbw::asset_type::AUDIO;
          case asset::SCENE: return cbw::asset_type::SCENE;
          case asset::SCENE_OBJECT: return cbw::asset_type::SCENE_OBJECT;
          default: return cbw::asset_type::UNKNOWN;
        }
      }

      std::string format_file_size(natural_t bytes) {
        constexpr natural_t kKB = 1024;
        constexpr natural_t kMB = 1024 * 1024;

        if (bytes >= kMB) {
          std::ostringstream oss;
          oss << std::fixed;
          oss.precision(1);
          oss << (static_cast<double>(bytes) / static_cast<double>(kMB)) << " MB";
          return oss.str();
        }

        if (bytes >= kKB) {
          std::ostringstream oss;
          oss << std::fixed;
          oss.precision(1);
          oss << (static_cast<double>(bytes) / static_cast<double>(kKB)) << " KB";
          return oss.str();
        }

        return std::to_string(bytes) + " B";
      }

    }  // namespace

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

      auto* fs = subsystem<file_system>::get();
      if (fs == nullptr) {
        return;
      }

      std::string mount_name;
      std::string relative_path;
      auto sep = current_path.find('/');
      if (sep == std::string::npos) {
        mount_name = current_path;
      } else {
        mount_name = current_path.substr(0, sep);
        relative_path = current_path.substr(sep + 1);
      }

      auto mount = fs->get_mount(mount_name);
      if (mount == nullptr) {
        return;
      }

      ref<directory> target_dir = mount;
      if (!relative_path.empty()) {
        auto components = directory::split_path(relative_path);
        for (const auto& comp : components) {
          target_dir = target_dir->get_child_directory(comp);
          if (target_dir == nullptr) {
            return;
          }
        }
      }

      for (const auto& child : target_dir->child_directories()) {
        auto sub_dirs = child->child_directories();
        auto sub_files = child->files();
        std::string meta = std::to_string(sub_dirs.size() + sub_files.size()) + " items";
        assets.push_back({ child->name(), meta, cbw::asset_type::FOLDER });
      }

      asset_handler* handler = nullptr;
      if (driver_ptr != nullptr) {
        handler = driver_ptr->get_asset_manager().get();
      }

      for (const auto& file : target_dir->files()) {
        const std::string& ext = file->extension();
        asset::type at = asset::get_type_from_extension(ext);
        cbw::asset_type ui_type = map_asset_to_ui_type(at);

        std::string meta;
        if (file->exists()) {
          meta = format_file_size(file->size());
        }

        if (handler != nullptr) {
          natural_t path_hash = FNV(file->absolute_path().string());
          asset_state state = handler->get_asset_state_by_path_hash(path_hash);
          if (state == asset_state::LOADED) {
            meta += " \xc2\xb7 loaded";
          } else if (state == asset_state::LOADING) {
            meta += " \xc2\xb7 loading";
          }
        }

        assets.push_back({ file->name(), meta, ui_type });
      }
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