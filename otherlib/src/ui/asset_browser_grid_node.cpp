/**
 * \file ui/asset_browser_grid_node.cpp
 **/
#include "ui/asset_browser_grid_node.hpp"

/**
 * \file ui/asset_browser_grid_node.cpp
 **/
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <unordered_set>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/subsystem.hpp"
#include "file/directory.hpp"
#include "file/file_handle.hpp"
#include "file/filesystem.hpp"

#include "renderer/ui/colors.hpp"
#include "renderer/ui/unicode.hpp"

#include "driver/driver.hpp"
#include "ui/asset_browser_grid_node.hpp"
#include "ui/asset_browser_widgets.hpp"
#include "ui/inspector_widgets.hpp"

#include "asset/asset.hpp"
#include "asset/asset_handler.hpp"

namespace other {
  namespace ui {

    namespace abw = asset_browser_w;

    namespace {

      abw::asset_type map_asset_to_ui_type(asset::type t) {
        switch (t) {
          case asset::TEXTURE: return abw::asset_type::TEXTURE;
          case asset::MODEL_SOURCE: return abw::asset_type::MODEL_SOURCE;
          case asset::MODEL: return abw::asset_type::MODEL;
          case asset::ANIMATION: return abw::asset_type::ANIMATION;
          case asset::SCRIPT_SOURCE: return abw::asset_type::SCRIPT_SOURCE;
          case asset::SCRIPT: return abw::asset_type::SCRIPT;
          case asset::AUDIO: return abw::asset_type::AUDIO;
          case asset::SCENE: return abw::asset_type::SCENE;
          case asset::SCENE_OBJECT: return abw::asset_type::SCENE_OBJECT;
          default: return abw::asset_type::UNKNOWN;
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
      // events().add_listener("asset-browser.refresh", [this](const value&) { refresh_listing(); });
      /// set up default filter pills
      filters = {
        { "All", abw::asset_type::UNKNOWN, true },
        { "Texture", abw::asset_type::TEXTURE, true },
        { "Model", abw::asset_type::MODEL, true },
        { "Model Src", abw::asset_type::MODEL_SOURCE, true },
        { "Animation", abw::asset_type::ANIMATION, true },
        { "Script Src", abw::asset_type::SCRIPT_SOURCE, true },
        { "Script", abw::asset_type::SCRIPT, true },
        { "Audio", abw::asset_type::AUDIO, true },
        { "Scene", abw::asset_type::SCENE, true },
        { "Object", abw::asset_type::SCENE_OBJECT, true },
      };

      navigate_to("assets");
    }

    void asset_browser_grid_node::navigate_to(const std::string& path) {
      CORE_LOG_DEBUG("Navigating to path: {}", path);
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

    static const char* asset_state_label(asset_state state) {
      switch (state) {
        case asset_state::UNLOADED: return "unloaded";
        case asset_state::LOADING: return "loading";
        case asset_state::LOADED: return "loaded";
        case asset_state::OUT_OF_DATE: return "out-of-date";
        case asset_state::UNLOADING: return "unloading";
        case asset_state::ERROR_STATE: return "error";
        default: return "unknown";
      }
    }

    void asset_browser_grid_node::rebuild_asset_list() {
      assets.clear();

      asset_handler* handler = nullptr;
      if (driver_ptr != nullptr) {
        handler = driver_ptr->get_asset_manager().get();
      }

      auto* fs = subsystem<file_system>::get();

      /// collect path hashes of assets found in the filesystem to avoid duplicates
      std::unordered_set<natural_t> fs_path_hashes;

      if (fs != nullptr) {
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
        if (mount != nullptr) {
          ref<directory> target_dir = mount;
          if (!relative_path.empty()) {
            auto components = directory::split_path(relative_path);
            for (const auto& comp : components) {
              target_dir = target_dir->get_child_directory(comp);
              if (target_dir == nullptr) {
                break;
              }
            }
          }

          if (target_dir != nullptr) {
            for (const auto& child : target_dir->child_directories()) {
              auto sub_dirs = child->child_directories();
              auto sub_files = child->files();
              std::string meta = std::to_string(sub_dirs.size() + sub_files.size()) + " items";
              assets.push_back({
                .name = child->name(),
                .meta = meta,
                .type = abw::asset_type::FOLDER,
              });
            }

            for (const auto& file : target_dir->files()) {
              const std::string& ext = file->extension();
              asset::type at = asset::get_type_from_extension(ext);
              abw::asset_type ui_type = map_asset_to_ui_type(at);

              std::string meta;
              if (file->exists()) {
                meta = format_file_size(file->size());
              }

              natural_t path_hash = FNV(file->absolute_path().string());
              fs_path_hashes.insert(path_hash);

              natural_t handler_asset_id = 0;
              if (handler != nullptr) {
                asset_state state = handler->get_asset_state_by_path_hash(path_hash);
                if (state != asset_state::UNLOADED) {
                  meta += std::string(std::format("{}", unicode::kMiddleDot)) + asset_state_label(state);
                }
                handler_asset_id = handler->get_asset_id_by_path_hash(path_hash);
              }

              assets.push_back({
                .name = file->name(),
                .meta = meta,
                .type = ui_type,
                .thumbnail_id = 0,
                .is_selected = false,
                .handler_asset_id = handler_asset_id,
                .asset_path = file->absolute_path().string(),
              });
            }
          }
        }
      }

      // /// include all handler-tracked assets that are not already in the filesystem listing
      // if (handler != nullptr) {
      //   auto tracked_ids = handler->get_all_tracked_ids();
      //   for (natural_t id : tracked_ids) {
      //     const asset* a = handler->get_loaded_asset(id);
      //     OTHER_ASSERT(a != nullptr, "Asset should not be null");

      //     asset_state state = handler->get_asset_state(id);

      //     filepath asset_path;
      //     asset::type atype = asset::EMPTY;

      //     if (fs_path_hashes.contains(a->path_hash)) {
      //       continue;
      //     }
      //     asset_path = a->path;
      //     atype = a->asset_type;

      //     std::string name = asset_path.filename().string();
      //     abw::asset_type ui_type = map_asset_to_ui_type(atype);

      //     std::string meta = asset_state_label(state);
      //     if (std::filesystem::exists(asset_path)) {
      //       auto sz = std::filesystem::file_size(asset_path);
      //       meta = format_file_size(sz) + std::format(" {} ", unicode::kMiddleDot) + meta;
      //     }

      //     abw::asset_card_desc desc;
      //     desc.name = std::move(name);
      //     desc.meta = std::move(meta);
      //     desc.type = ui_type;
      //     desc.handler_asset_id = id;
      //     desc.asset_path = asset_path.string();
      //     assets.push_back(std::move(desc));
      //   }
      // }
    }

    bool asset_browser_grid_node::passes_filter(const abw::asset_card_desc& desc) const {
      /// folders always show
      if (desc.type == abw::asset_type::FOLDER) {
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
      refresh_listing();
      ImDrawList* dl = ImGui::GetWindowDrawList();

      {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;

        dl->AddRectFilled(cursor, { cursor.x + w, cursor.y + abw::kToolbarHeight }, to_im_col(asset_browser::kBG));
        dl->AddLine(
          { cursor.x, cursor.y + abw::kToolbarHeight },
          { cursor.x + w, cursor.y + abw::kToolbarHeight },
          to_im_col(asset_browser::kBorder), 1.f
        );

        ImGui::SetCursorScreenPos({ cursor.x + abw::kPaddingX, cursor.y + abw::kPaddingY });

        int clicked_crumb = abw::draw_breadcrumbs(breadcrumbs);
        if (clicked_crumb >= 0 && clicked_crumb < static_cast<int>(breadcrumbs.size())) {
          navigate_to(breadcrumbs[clicked_crumb].full_path);
        }

        /// search field on the right
        float search_w = 200.f;
        ImGui::SameLine(0.f, 0.f);
        float search_x = cursor.x + w - search_w - abw::kPaddingX;
        ImGui::SetCursorScreenPos({ search_x, cursor.y + abw::kPaddingY });
        if (abw::draw_search_input(search_buf, sizeof(search_buf), search_w)) {
          search_lower = search_buf;
          std::ranges::transform(search_lower, search_lower.begin(), [](unsigned char c) { return std::tolower(c); });
        }

        ImGui::SetCursorScreenPos({ cursor.x, cursor.y + abw::kToolbarHeight + 1.f });
      }

      {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x;

        dl->AddRectFilled(cursor, { cursor.x + w, cursor.y + abw::kFilterBarHeight }, to_im_col(colors::kBG0));
        dl->AddLine(
          { cursor.x, cursor.y + abw::kFilterBarHeight },
          { cursor.x + w, cursor.y + abw::kFilterBarHeight },
          to_im_col(asset_browser::kBorder), 1.f
        );

        ImGui::SetCursorScreenPos({ cursor.x + abw::kPaddingX, cursor.y + 5.f });

        int visible_count = 0;
        for (const auto& a : assets) {
          if (passes_filter(a)) {
            ++visible_count;
          }
        }
        abw::draw_filter_bar(filters, visible_count);

        ImGui::SetCursorScreenPos({ cursor.x, cursor.y + abw::kFilterBarHeight + 1.f });
      }

      float card_w = abw::card_width_from_zoom(zoom);
      float avail_w = ImGui::GetContentRegionAvail().x;

      /// reserve space for the detail panel when a tracked asset is selected
      constexpr float kDetailPanelHeight = 160.f;
      bool show_detail_panel = false;
      if (selected_asset_idx >= 0 && selected_asset_idx < static_cast<int>(assets.size()) &&
          assets[selected_asset_idx].handler_asset_id != 0) {
        show_detail_panel = true;
        ImGui::OpenPopup("##asset-detail-popup");
      }

      float detail_h = show_detail_panel ? kDetailPanelHeight : 0.f;
      float avail_h = ImGui::GetContentRegionAvail().y - abw::kStatusBarHeight - detail_h;

      ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(asset_browser::kBG));
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(abw::kPaddingX, abw::kPaddingX));

      float top_of_grid_y = ImGui::GetCursorScreenPos().y;
      if (ImGui::BeginChild("##asset-grid-scroll", ImVec2(0, avail_h), ImGuiChildFlags_None)) {
        int cols = std::max(1, static_cast<int>((avail_w - abw::kPaddingX) / (card_w + abw::kCardSpacing)));
        int col = 0;

        size_t idx = 0;
        for (auto& asset : assets) {
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

          ImGui::PushID(idx);

          if (col > 0) {
            ImGui::SameLine(0.f, abw::kCardSpacing);
          }

          if (abw::draw_asset_card(asset, card_w)) {
            /// deselect all others
            for (int j = 0; j < static_cast<int>(assets.size()); ++j) {
              assets[j].is_selected = (j == static_cast<int>(idx));
            }
            selected_asset_idx = static_cast<int>(idx);

            if (asset.type == abw::asset_type::FOLDER) {
              navigate_to(current_path + "/" + asset.name);
            }
          }

          ImGui::PopID();
          idx++;

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
          if (a.type == abw::asset_type::FOLDER) {
            folder_count++;
          } else {
            asset_count++;
          }
        }

        abw::status_bar_info info;
        info.total_assets = asset_count;
        info.folder_count = folder_count;
        info.selected_name = (selected_asset_idx >= 0 && selected_asset_idx < static_cast<int>(assets.size())) ? assets[selected_asset_idx].name.c_str() : nullptr;
        info.zoom_normalized = zoom;

        if (abw::draw_status_bar(info)) {
          zoom = info.zoom_normalized;
        }
      }

      /// inspector-style detail panel for handler-tracked assets

      /// \todo move this to a separate window or popup
      if (show_detail_panel && ImGui::BeginChild("##asset-detail-popup")) {
        const auto& sel = assets[selected_asset_idx];

        // ImGui::PushStyleColor(ImGuiCol_ChildBg, rgba_to_imvec4(colors::kBG0));
        // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(ui::inspector::kInnerPadding, 6.f));

        constexpr float kDetailPanelMinWidth = 240.f;
        constexpr float kDetailPanelMaxWidth = 400.f;
        constexpr float kDetailPanelDefaultWidth = 280.f;
        float detail_panel_width = std::clamp(avail_w * 0.4f, kDetailPanelMinWidth, kDetailPanelMaxWidth);
        detail_panel_width = std::max(detail_panel_width, kDetailPanelDefaultWidth);
        dl->AddLine(
          ImGui::GetCursorScreenPos(),
          { ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y },
          to_im_col(asset_browser::kBorder), 1.f
        );
        ImGui::Dummy(ImVec2(0, 2.f));

        asset_state state = asset_state::UNLOADED;

        /// \todo is there a better way to do this?
        if (driver_ptr != nullptr) {
          auto& handler = driver_ptr->get_asset_manager();
          OTHER_ASSERT(handler != nullptr, "Asset handler should not be null");

          state = handler->get_asset_state(sel.handler_asset_id);
        }

        ui::inspector::asset_slot_state slot_state = ui::inspector::asset_slot_state::EMPTY;
        if (state == asset_state::LOADED) {
          slot_state = ui::inspector::asset_slot_state::FILLED;
        } else if (state == asset_state::ERROR_STATE) {
          slot_state = ui::inspector::asset_slot_state::INVALID;
        }

        std::string id_str = std::to_string(sel.handler_asset_id);
        const char* type_label = abw::badge_for_asset_type(sel.type);
        glm::vec4 type_color = abw::color_for_asset_type(sel.type);

        ui::inspector::draw_asset_slot("Asset", sel.name, slot_state);
        ui::inspector::property_display("ID", id_str);
        ui::inspector::property_display("Type", type_label, type_color);
        ui::inspector::property_display("State", asset_state_label(state));
        if (!sel.asset_path.empty()) {
          ui::inspector::property_display("Path", sel.asset_path);
        }

        // ImGui::PopStyleVar();
        // ImGui::PopStyleColor();
      }
      if (show_detail_panel) {
        ImGui::EndChild();
      }
    }

  }  // namespace ui
}  // namespace other