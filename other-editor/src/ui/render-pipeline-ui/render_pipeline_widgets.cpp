/**
 * \file ui/render-pipeline-ui/render_pipeline_widgets.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/ui_helpers.hpp"

namespace other {
  namespace ui {
    namespace inspector {

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