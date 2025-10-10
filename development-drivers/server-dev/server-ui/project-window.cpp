/**
 * \file server-dev/server-ui/project-window.cpp
 **/
#include "project-window.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/logger.hpp"

#include "renderer/ui/ui_helpers.hpp"
#include "renderer/ui/ui_node.hpp"

#include "imgui.h"

namespace other {
  namespace {

    ImU32 color_with_multiplier(const ImColor& color, float multiplier) {
      const ImVec4& color_val = color.Value;
      float hue, sat, val;
      ImGui::ColorConvertRGBtoHSV(color_val.x, color_val.y, color_val.z, hue, sat, val);
      return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.f));
    }

    void table_headers() {
      ImColor bg_color = ImColor(33, 33, 33, 255);
      const ImColor active_color = color_with_multiplier(bg_color, 1.2f);
      scoped_color_stack color_stack(ImGuiCol_TableHeaderBg, bg_color);

      const float row_height = ImGui::TableGetHeaderRowHeight();
      ImGui::TableNextRow(ImGuiTableRowFlags_Headers, row_height);

      const int columns_count = ImGui::TableGetColumnCount();
      for (int column_n = 0; column_n < columns_count; column_n++) {
        if (!ImGui::TableSetColumnIndex(column_n)) {
          continue;
        }

        const char* name = (ImGui::TableGetColumnFlags(column_n) & ImGuiTableColumnFlags_NoHeaderLabel) ? "" : ImGui::TableGetColumnName(column_n);
        ImGui::PushID(column_n);
        ImGui::TableHeader(name);
        ImGui::PopID();

        underline(false, 0.f, 5.f);
      }
    }

    struct project_list_node : public ui_node {
      enum tool {
        TOOL_CREATE_PROJECT,
        TOOL_REMOVE_PROJECT,
        TOOL_SETTINGS,

        NUM_TOOLS,
        INVALID_TOOL = NUM_TOOLS
      };

      project_list_node(ui_window* parent, json::json& project_cache)
          : ui_node(parent, "project-list"), project_cache(project_cache) {
        // clang-format off
        kTools[TOOL_CREATE_PROJECT] = [this]() { events().trigger_event("goto-create-project-page"); };
        kTools[TOOL_REMOVE_PROJECT] = [this]() { events().trigger_event("remove-project"); };
        kTools[TOOL_SETTINGS] = [this]() { events().trigger_event("goto-settings-page"); };
      }
      virtual ~project_list_node() = default;

      static constexpr size_t kNumTools = NUM_TOOLS;
      std::array<std::function<void()>, kNumTools> kTools;

      void render_node() override {
        if (ImGui::BeginChild("##project-table-child", ImVec2(0.f, 512.f))) {
          render_project_table();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("##project-toolbar-child", ImVec2(0.f, 64.f))) {
          render_toolbar_table();
        }
        ImGui::EndChild();

        if (ImGui::BeginChild("##project-console-child")) {
          render_console_table();
        }
        ImGui::EndChild();
      }

      void render_project_node(const json::json& p, const ImColor& dark_bg_color, const ImColor& hover_highlight) {
        constexpr float row_height = 64.f;
        constexpr float text_padding_y = 20.f;
        constexpr float button_vert_offset = 3.f;
        constexpr float row_dist_factor = 6.66f;
        constexpr float magic_number = 7.616f;
        ImGui::TableNextRow(0, row_height);
        ImGui::TableSetColumnIndex(0);

        shift_cursor_y(text_padding_y);
        float cursor_y = ImGui::GetCursorPosY();

        ImGui::TextWrapped("  [%s] @ %s", p["name"].get<std::string>().c_str(), p["working-directory"].get<std::string>().c_str());

        std::string proj_file = p.contains("project-file") ? p["project-file"].get<std::string>() : "No project file";
        ImGui::TextWrapped("  %s", proj_file.c_str());

        /// now in line with where the original text started (a little below)
        ///   we go all the way to the other side of the table-row and draw the button
        ImGui::SetCursorPosY(cursor_y - button_vert_offset);

        float col_width = ImGui::GetColumnWidth(0);
        /// \todo choose betweeen large factor and small magic number depending on col width
        shift_cursor_x(row_dist_factor * col_width / magic_number);  // magic number to get it to the right spot

        {
          scoped_color button_color(ImGuiCol_Button, dark_bg_color.Value);
          scoped_color button_hover_color(ImGuiCol_ButtonHovered, hover_highlight.Value);
          if (ImGui::Button(("Open##" + p["name"].get<std::string>()).c_str(), ImVec2(100.f, 32.f))) {
            events().set_user_data("open-project", p["name"].get<std::string>());
            events().trigger_event("open-project");
          }
        }
      }

      void render_project_table() {
        ImVec2 content_region = ImGui::GetContentRegionAvail();
        ImVec2 project_child_size = ImVec2(content_region.x, 512.f);

        constexpr auto background = IM_COL32(36, 36, 36, 255);
        ImColor bg_color = ImColor(background);

        constexpr auto dark_background = IM_COL32(22, 22, 22, 255);
        ImColor dark_bg_color = ImColor(dark_background);

        constexpr auto highlight = IM_COL32(39, 185, 242, 255);

        scoped_style style(ImGuiStyleVar_CellPadding, ImVec2(4.f, 0.f));
        const ImColor col_row_alt = color_with_multiplier(bg_color, 1.2f);

        scoped_color row_color(ImGuiCol_TableRowBg, bg_color.Value);
        scoped_color row_alt_color(ImGuiCol_TableRowBgAlt, col_row_alt.Value);
        scoped_color table_color(ImGuiCol_ChildBg, dark_bg_color.Value);

        ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_ScrollY;

        size_t columns = 1;
        std::string headers[] = { "Projects" };
        if (!ImGui::BeginTable("##project-table", columns, flags, ImVec2(project_child_size.x, project_child_size.y))) {
          return;
        }

        for (uint32_t i = 0; i < columns; ++i) {
          ImGui::TableSetupColumn(headers[i].c_str());
        }

        table_headers();

        for (const auto& p : project_cache["projects"]) {
          ImGui::PushID(p["name"].get<std::string>().c_str());
          render_project_node(p, dark_bg_color, ImColor(highlight));
          ImGui::PopID();
        }

        ImGui::EndTable();
      }

      void render_tool_button(uint32_t tool_idx, const std::string& tool_name, const ImColor& dark_bg_color, const ImColor& hover_highlight) {
        scoped_color button_color(ImGuiCol_Button, dark_bg_color.Value);
        scoped_color button_hover_color(ImGuiCol_ButtonHovered, hover_highlight.Value);

        ImVec2 avail = ImGui::GetContentRegionAvail();

        float tool_name_width = ImGui::CalcTextSize(tool_name.c_str()).x;
        /// center the button

        /// put in middle
        ImVec2 button_size = ImVec2(tool_name_width + 16.f, 32.f);
        float half_button_width = button_size.x / 2.f;
        float half_button_height = button_size.y / 2.f;

        float half_width = avail.x / 2.f;
        shift_cursor_x(half_width - half_button_width);

        float half_height = avail.y / 2.f;
        shift_cursor_y((half_height - half_button_height) + 4.f);

        if (ImGui::Button(tool_name.c_str(), button_size)) {
          OTHER_ASSERT(tool_idx < kNumTools, "Invalid tool index {}", tool_idx);
          OTHER_ASSERT(kTools[tool_idx] != nullptr, "No tool action defined for tool index {}", tool_idx);
          kTools[tool_idx]();
        }
      }

      void render_tool(uint32_t tool_idx, const ImColor& dark_bg_color = ImColor(22, 22, 22, 255), const ImColor& hover_highlight = ImColor(236, 158, 36, 255)) {
        scoped_color button_color(ImGuiCol_Button, dark_bg_color.Value);
        scoped_color button_hover_color(ImGuiCol_ButtonHovered, hover_highlight.Value);

        switch (tool_idx) {
          case TOOL_CREATE_PROJECT: render_tool_button(tool_idx, "Create Project", dark_bg_color, hover_highlight); break;
          case TOOL_REMOVE_PROJECT: render_tool_button(tool_idx, "Remove Project", dark_bg_color, hover_highlight); break;
          case TOOL_SETTINGS: render_tool_button(tool_idx, "Settings", dark_bg_color, hover_highlight); break;
          default:
            break;
        }
      }

      void render_toolbar_table() {
        if (!ImGui::BeginTable("##project-toolbar-table", 3, ImGuiTableFlags_NoBordersInBody)) {
          return;
        }

        size_t columns = 3;  // will be num tools/tool-menus
        std::string headers[] = { "Add Project", "Remove Project", "Settings" };
        for (uint32_t i = 0; i < columns; ++i) {
          ImGui::TableSetupColumn(headers[i].c_str(), ImGuiTableColumnFlags_NoHeaderLabel);
        }

        table_headers();
        ImGui::TableNextRow();

        for (uint32_t col = 0; col < columns; ++col) {
          /// get tool-data here and render icon/button/menu
          ImGui::TableSetColumnIndex(col);
          ImGui::PushID(col);
          render_tool(col);
          ImGui::PopID();
        }

        ImGui::EndTable();
      }

      void render_console_table() {
        if (!ImGui::BeginTable("##project-toolbar-table", 3, ImGuiTableFlags_NoBordersInBody)) {
          return;
        }

        /// \todo figure out how to handle this better
        size_t columns = 1;
        std::string headers[] = { "" };  // <nothing> { "" };
        for (uint32_t i = 0; i < columns; ++i) {
          ImGui::TableSetupColumn(headers[i].c_str(), ImGuiTableColumnFlags_NoHeaderLabel);
        }

        table_headers();

        ImGui::EndTable();
      }

      json::json& project_cache;
    };

  }  // namespace

  project_window::project_window(event_system& events, json::json& project_cache)
      : ui_window(events, "Projects"), project_cache(project_cache) {
    add_node(make_scope<project_list_node>(this, project_cache));
  }

}  // namespace other