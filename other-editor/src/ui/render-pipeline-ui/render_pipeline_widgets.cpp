/**
 * \file ui/render-pipeline-ui/render_pipeline_widgets.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"

#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/profiler.hpp"
#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/ui_helpers.hpp"
#include "ui/unicode.hpp"

namespace other {
  namespace ui {
    namespace inspector {

      std::vector<render_pipeline_data> rebuild_render_pipeline_list(const renderer& r, const asset_system& assets) {
        PROFILE_SECTION("rebuild_render_pipeline_list");
        std::vector<render_pipeline_data> entries;

        for (const std::string& name : r.get_pipeline_names()) {
          entries.push_back({
            .name = name,
            .live = true,
            .on_disk = false,
            .path = {},
          });
        }

        for (const asset* a : assets.get_assets_of_type(asset::RENDERING_PIPELINE)) {
          if (auto e = std::ranges::find_if(entries, [&](const render_pipeline_data& d) { return d.name == a->virtual_path.stem().string(); });
              e != entries.end()) {
            e->on_disk = true;
            e->path = a->virtual_path;
          } else {
            entries.push_back({
              .name = a->virtual_path.stem().string(),
              .live = false,
              .on_disk = true,
              .path = a->virtual_path,
            });
          }
        }

        std::ranges::sort(entries, [](auto& x, auto& y) { return x.name < y.name; });
        return entries;
      }

      int32_t draw_render_pipeline_list(const std::vector<render_pipeline_data>& entries, int32_t selected_index, int32_t pending_select_index, bool current_dirty) {
        PROFILE_SECTION("draw_render_pipeline_list");
        static constexpr glm::vec4 kLiveDot = colors::hex_col_to_rgba(IM_COL32(90, 200, 120, 255));
        static constexpr glm::vec4 kDiskDot = colors::hex_col_to_rgba(IM_COL32(140, 140, 140, 255));
        static constexpr float kTitleHeaderHeight = 24.f;
        static constexpr float kListHeight = 140.f;

        if (!ImGui::BeginChild("pl-list", ImVec2(0.f, kListHeight))) {
          ImGui::EndChild();
          return pending_select_index;
        }

        /// place a dark background behind the list
        auto* draw_list = ImGui::GetWindowDrawList();
        OTHER_ASSERT(draw_list != nullptr, "ImGui::GetWindowDrawList() returned null in render_pipeline_viewer::draw_list");
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 current_child_size = ImGui::GetContentRegionAvail();

        // list background
        {
          ImVec2 p1 = ImVec2(p0.x + current_child_size.x, p0.y + current_child_size.y);
          draw_list->AddRectFilled(p0, p1, colors::rgba_to_hex(colors::kBG1), ui::inspector::kRenderPipelineListUiRounding);
        }
        // list title header
        {
          ImVec2 p1 = ImVec2(p0.x + current_child_size.x, p0.y + kTitleHeaderHeight);
          draw_list->AddRectFilled(p0, p1, colors::rgba_to_hex(colors::kBG2), ui::inspector::kRenderPipelineListUiRounding);
          ImGui::SetCursorScreenPos(ImVec2(p0.x + 8.f, p0.y + 4.f));
          ImGui::Text("Pipelines");
        }

        for (size_t i = 0; i < entries.size(); ++i) {
          const auto& e = entries[i];
          scoped_id row_id{ i };
          {
            scoped_color dot{ ImGuiCol_Text, colors::rgba_to_imvec4(e.live ? kLiveDot : kDiskDot) };
            std::string mod_text_str = std::string(e.live ? unicode::kStatusDot : unicode::kHollowDot);  // ● / ○
            ImGui::Text("%s", mod_text_str.c_str());
          }
          ImGui::SameLine();

          const bool is_current = (i == selected_index);
          std::string label = e.name;
          if (is_current && current_dirty) {
            label += " *";
          }

          if (ImGui::Selectable(label.c_str(), is_current, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
            if (is_current) {
              pending_select_index = selected_index;
            } else {
              pending_select_index = i;
            }
          }

          // right-aligned source tag
          std::string tag = " ";
          if (e.live) {
            tag += "live ";
          } else {
            tag += "     ";
          }
          if (e.on_disk) {
            tag += "disk";
          } else {
            tag += "    ";
          }
          tag += " ";
          ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(tag.c_str()).x - 4.f);
          ImGui::TextDisabled("%s", tag.c_str());
        }

        ImGui::EndChild();
        return pending_select_index;
      }

      static bool s_component_section_open = true;
      bool begin_pipeline_properties(const std::string_view title, natural_t id) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 cursor = ImGui::GetCursorScreenPos();
        const float avail_w = ImGui::GetContentRegionAvail().x;

        ImGui::PushID(id);

        ImVec2 header_min = cursor;
        ImVec2 header_max = { cursor.x + avail_w, cursor.y + kComponentHeaderHeight };

        bool hovered = ImGui::IsMouseHoveringRect(header_min, header_max);
        ImU32 header_bg = colors::to_im_col(hovered ? colors::inspector::kComponentHeaderHover : colors::inspector::kComponentHeaderBG);
        dl->AddRectFilled(header_min, header_max, header_bg, kRenderPipelineListUiRounding);

        float dot_cx = cursor.x + kInnerPadding;
        float dot_cy = cursor.y + kComponentHeaderHeight * 0.5f;
        float text_x = dot_cx + kComponentDotRadius + 8.f;
        float text_y = cursor.y + (kComponentHeaderHeight - ImGui::GetFontSize()) * 0.5f;

        draw_filled_circle(dl, { dot_cx, dot_cy }, kComponentDotRadius, colors::to_im_col(colors::kHeader));
        dl->AddText({ text_x, text_y }, colors::to_im_col(colors::inspector::kComponentHeaderText), title.data());

        // if (flags.modified) {
        //   std::string mod_text_str = std::string(unicode::kStatusDot) + " modified";
        //   const char* mod_text = mod_text_str.c_str();

        //   float mod_w = ImGui::CalcTextSize(mod_text).x;
        //   float mod_x = cursor.x + avail_w - kInnerPadding - mod_w;
        //   dl->AddText({ mod_x, text_y }, colors::to_im_col(colors::inspector::kModifiedMarker), mod_text);
        // }

        // if (flags.removable && out_remove_requested) {
        //   float btn_x = cursor.x + avail_w - kInnerPadding - (flags.modified ? 80.f : 0.f) - 16.f;
        //   float btn_y = cursor.y + (kComponentHeaderHeight - 12.f) * 0.5f;
        //   ImGui::SetCursorScreenPos({ btn_x, btn_y });
        //   ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        //   ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::rgba_to_imvec4(colors::inspector::kComponentHeaderHover));
        //   ImGui::PushStyleColor(ImGuiCol_Text, colors::rgba_to_imvec4(colors::inspector::kResetButton));
        //   if (ImGui::SmallButton("x")) {
        //     *out_remove_requested = true;
        //   }
        //   ImGui::PopStyleColor(3);
        // }

        /// click to toggle
        ImGui::SetCursorScreenPos(header_min);
        std::string btn_id = std::format("##comp_hdr_{}", title);
        if (ImGui::InvisibleButton(btn_id.c_str(), { avail_w, kComponentHeaderHeight })) {
          /// toggle stored in ImGui internal storage
          ImGuiStorage* storage = ImGui::GetStateStorage();
          ImGuiID state_id = ImGui::GetID("##comp_open");
          bool was_open = storage->GetBool(state_id, true);
          storage->SetBool(state_id, !was_open);
        }

        {
          ImGuiStorage* storage = ImGui::GetStateStorage();
          ImGuiID state_id = ImGui::GetID("##comp_open");
          s_component_section_open = storage->GetBool(state_id, true);
        }

        ImGui::SetCursorScreenPos({ cursor.x, header_max.y + 1.f });
        ImGui::PopID();
        return s_component_section_open;
      }

      void end_pipeline_properties() {
      }

    }  // namespace inspector
  }  // namespace ui
}  // namespace other