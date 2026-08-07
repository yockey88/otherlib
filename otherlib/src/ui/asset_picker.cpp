/**
 * \file ui/asset_picker.cpp
 **/
#include "ui/asset_picker.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include <imgui/imgui.h>

#include "core/profiler.hpp"
#include "core/subsystem.hpp"
#include "file/directory.hpp"
#include "file/file_handle.hpp"
#include "file/filesystem.hpp"

#include "driver/driver.hpp"
#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"

#include "asset/asset_handler.hpp"

namespace other {
  namespace ui {
    namespace inspector {

      namespace {

        struct picker_entry {
          std::string name;
          std::string rel_dir;
          filepath abs_path;
        };

        void collect_asset_files(const ref<directory>& dir, const filepath& mount_root, asset::type type, std::vector<picker_entry>& out) {
          if (dir == nullptr) {
            return;
          }
          for (const auto& file : dir->files()) {
            const asset::type file_type = asset::get_type_from_extension(file->extension());
            if (file_type == asset::EMPTY || (type != asset::EMPTY && file_type != type)) {
              continue;
            }
            filepath rel = file->absolute_path().lexically_relative(mount_root);
            out.push_back({ file->name(), rel.parent_path().generic_string(), file->absolute_path() });
          }
          for (const auto& child : dir->child_directories()) {
            collect_asset_files(child, mount_root, type, out);
          }
        }

        std::string to_lower(const std::string_view s) {
          std::string out{ s };
          std::ranges::transform(out, out.begin(), [](unsigned char c) { return std::tolower(c); });
          return out;
        }

        std::string display_name_for(natural_t asset_id, asset_handler* handler, asset_slot_state& out_state) {
          if (asset_id == 0) {
            out_state = asset_slot_state::EMPTY;
            return "None";
          }
          if (handler == nullptr) {
            out_state = asset_slot_state::FILLED;
            return std::format("Asset #{}", asset_id);
          }
          if (!handler->asset_exists(asset_id)) {
            out_state = asset_slot_state::INVALID;
            return std::format("Missing #{}", asset_id);
          }
          if (const asset* a = handler->get_loaded_asset(asset_id); a != nullptr) {
            out_state = asset_slot_state::FILLED;
            return a->load_path.filename().string();
          }
          if (handler->get_asset_state(asset_id) == asset_state::ERROR_STATE) {
            out_state = asset_slot_state::INVALID;
            return "Load Error";
          }
          out_state = asset_slot_state::LOADING;
          if (const asset* a = handler->get_asset(asset_id); a != nullptr) {
            return a->load_path.filename().string();
          }
          return "Loading...";
        }

        glm::vec4 slot_border_color(asset_slot_state state) {
          switch (state) {
            case asset_slot_state::EMPTY: return colors::scene_object::kAssetSlotEmpty;
            case asset_slot_state::DRAG_HOVER: return colors::scene_object::kAssetSlotDragHover;
            case asset_slot_state::FILLED: return colors::scene_object::kAssetSlotFilled;
            case asset_slot_state::LOADING: return colors::scene_object::kAssetSlotLoading;
            case asset_slot_state::INVALID:
            default:
              return colors::scene_object::kAssetSlotInvalid;
          }
        }

      }  // namespace

      bool property_asset_field(const std::string_view label, natural_t& asset_id, asset::type type, asset_handler* handler, driver* drvr) {
        OTHER_ASSERT(drvr != nullptr, "property_asset_field('{}') needs a driver", label);
        PROFILE_SECTION("property_asset_field");
        bool changed = false;

        ImGui::PushID(label.data());

        asset_slot_state state = asset_slot_state::EMPTY;
        const std::string display = display_name_for(asset_id, handler, state);

        begin_property_row(label);

        constexpr float kClearWidth = 20.f;
        const bool show_clear = asset_id != 0;
        float button_w = ImGui::CalcItemWidth() - (show_clear ? kClearWidth + 2.f : 0.f);

        ImGui::PushStyleColor(ImGuiCol_Button, colors::rgba_to_imvec4(colors::inspector::kFieldBG));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::rgba_to_imvec4(colors::inspector::kComponentHeaderHover));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors::rgba_to_imvec4(colors::inspector::kComponentHeaderHover));
        ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kPropertyValueText));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);

        /// OpenPopup must run outside the row's PushID so it matches BeginPopup below
        bool open_picker = ImGui::Button(display.c_str(), ImVec2(button_w, 0.f));

        /// accept drops from the asset browser
        if (ImGui::BeginDragDropTarget()) {
          if (const ImGuiPayload* peek = ImGui::AcceptDragDropPayload(kAssetDragDropPayloadType, ImGuiDragDropFlags_AcceptPeekOnly); peek != nullptr) {
            const auto* data = static_cast<const asset_drag_drop_payload*>(peek->Data);
            if (data != nullptr && (type == asset::EMPTY || data->asset_type == type)) {
              state = asset_slot_state::DRAG_HOVER;
            }
          }
          if (const ImGuiPayload* dropped = ImGui::AcceptDragDropPayload(kAssetDragDropPayloadType); dropped != nullptr) {
            const auto* data = static_cast<const asset_drag_drop_payload*>(dropped->Data);
            if (data != nullptr && (type == asset::EMPTY || data->asset_type == type)) {
              natural_t new_id = data->handler_asset_id;
              if (new_id == 0 && data->path[0] != '\0') {
                new_id = drvr->begin_asset_load(filepath{ data->path });
              }
              if (new_id != 0 && new_id != asset_id) {
                asset_id = new_id;
                changed = true;
              }
            }
          }
          ImGui::EndDragDropTarget();
        }

        {
          ImDrawList* dl = ImGui::GetWindowDrawList();
          dl->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), colors::to_im_col(slot_border_color(state)), 3.f, 0, 1.5f);
        }

        if (show_clear) {
          ImGui::SameLine(0.f, 2.f);
          if (ImGui::Button("x", ImVec2(kClearWidth, 0.f))) {
            asset_id = 0;
            changed = true;
          }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        end_property_row();

        if (open_picker) {
          ImGui::OpenPopup("##asset_pick");
        }

        ImGui::SetNextWindowSize(ImVec2(420.f, 340.f), ImGuiCond_Appearing);
        if (ImGui::BeginPopup("##asset_pick")) {
          /// one picker popup is open at a time, so a single shared cache is safe
          static char search_buf[128] = {};
          static std::vector<picker_entry> entries;

          if (ImGui::IsWindowAppearing()) {
            PROFILE_SECTION("property_asset_field--scan_assets");
            search_buf[0] = '\0';
            entries.clear();
            if (auto* fs = subsystem<file_system>::get(); fs != nullptr) {
              if (ref<directory> mount = fs->get_mount("assets"); mount != nullptr) {
                collect_asset_files(mount, mount->absolute_path(), type, entries);
              }
            }
            std::ranges::sort(entries, [](const picker_entry& a, const picker_entry& b) { return a.name < b.name; });
            ImGui::SetKeyboardFocusHere();
          }

          ImGui::SetNextItemWidth(-FLT_MIN);
          ImGui::InputTextWithHint("##asset_pick_search", "search assets...", search_buf, sizeof(search_buf));
          ImGui::Separator();

          if (ImGui::BeginChild("##asset_pick_list")) {
            PROFILE_SECTION("property_asset_field--list_body");
            if (ImGui::Selectable("<None>")) {
              if (asset_id != 0) {
                asset_id = 0;
                changed = true;
              }
              ImGui::CloseCurrentPopup();
            }

            const std::string needle = to_lower(search_buf);
            uint32_t shown = 0;
            for (const auto& entry : entries) {
              if (!needle.empty() &&
                  to_lower(entry.name).find(needle) == std::string::npos &&
                  to_lower(entry.rel_dir).find(needle) == std::string::npos) {
                continue;
              }
              ++shown;

              std::string row = entry.rel_dir.empty() ? entry.name : std::format("{}  ({})", entry.name, entry.rel_dir);
              if (ImGui::Selectable(row.c_str())) {
                const natural_t new_id = drvr->begin_asset_load(entry.abs_path);
                if (new_id != 0 && new_id != asset_id) {
                  asset_id = new_id;
                  changed = true;
                }
                ImGui::CloseCurrentPopup();
              }
            }

            if (shown == 0) {
              const std::string_view type_name = type != asset::EMPTY ? kAssetTypeNames[static_cast<size_t>(type)].display_name : "matching";
              ImGui::TextDisabled("no %.*s assets found under assets://", static_cast<int>(type_name.size()), type_name.data());
            }
          }
          ImGui::EndChild();
          ImGui::EndPopup();
        }

        ImGui::PopID();
        return changed;
      }

    }  // namespace inspector
  }  // namespace ui
}  // namespace other
