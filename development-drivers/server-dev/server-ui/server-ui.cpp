/**
 * \file server-dev/server-ui/server-ui.cpp
 **/
#include "server-ui.hpp"

#include <string>

#include <imgui/imgui.h>

#include "renderer/ui/ui_helpers.hpp"

#include "project-creator.hpp"
#include "project-window.hpp"

/*
  constexpr auto no_color = IM_COL32(0 , 0 , 0 , 0);

  constexpr auto red = IM_COL32(255 , 0 , 0 , 255);
  constexpr auto green = IM_COL32(0 , 255 , 0 , 255);
  constexpr auto blue = IM_COL32(0 , 0 , 255 , 255);

  constexpr auto accent = IM_COL32(236 , 158 , 36 , 255);
  constexpr auto highlight = IM_COL32(39 , 185 , 242 , 255);
---
constexpr auto nice_blue = IM_COL32(83, 232, 254, 255);
constexpr auto compliment = IM_COL32(78, 151, 166, 255);
constexpr auto background = IM_COL32(36, 36, 36, 255);
constexpr auto background_dark = IM_COL32(26, 26, 26, 255);
constexpr auto title_bar = IM_COL32(21, 21, 21, 255);
constexpr auto title_bar_orange = IM_COL32(186, 66, 30, 255);
constexpr auto title_bar_green = IM_COL32(18, 88, 30, 255);
constexpr auto title_bar_red = IM_COL32(185, 30, 30, 255);
constexpr auto property_field = IM_COL32(15, 15, 15, 255);
constexpr auto text = IM_COL32(192, 192, 192, 255);
constexpr auto text_bright = IM_COL32(210, 210, 210, 255);
constexpr auto text_dark = IM_COL32(128, 128, 128, 255);
constexpr auto text_error = IM_COL32(230, 51, 51, 255);
constexpr auto muted = IM_COL32(77, 77, 77, 255);
constexpr auto group_header = IM_COL32(47, 47, 47, 255);
constexpr auto selection = IM_COL32(237, 192, 119, 255);
constexpr auto selection_muted = IM_COL32(237, 201, 142, 23);
constexpr auto background_popup = IM_COL32(50, 50, 50, 255);
constexpr auto valid_prefab = IM_COL32(82, 179, 222, 255);
constexpr auto invalid_prefab = IM_COL32(222, 43, 43, 255);
constexpr auto missing_mesh = IM_COL32(230, 102, 76, 255);
constexpr auto mesh_not_set = IM_COL32(250, 101, 23, 255);
*/

namespace other {
  namespace detail {

    struct ui_window_end_helper {
      ~ui_window_end_helper() { ImGui::End(); }
    };

  }  // namespace detail

  server_ui::server_ui(scope<renderer>& renderer_ptr, scope<event_system>& events, json::json& project_cache)
      : renderer_ptr(renderer_ptr), events(events), project_cache(project_cache) {
    project_win = make_scope<project_window>(*events, project_cache);
    project_win->initialize();

    create_project_win = make_scope<project_creator>(*events);
    create_project_win->initialize();

    events->register_event("create-project");
    events->add_listener("create-project", [this](const value& data) { state_machine.handle_event(ui_event::UI_EVENT_GO_TO_CREATE_PROJECT_PAGE); });

    events->register_event("goto-project-page");
    events->add_listener("goto-project-page", [this](const value& data) { state_machine.handle_event(ui_event::UI_EVENT_GO_TO_PROJECT_PAGE); });

    events->register_event("goto-create-project-page");
    events->add_listener("goto-create-project-page", [this](const value& data) { state_machine.handle_event(ui_event::UI_EVENT_GO_TO_CREATE_PROJECT_PAGE); });

    events->register_event("goto-settings-page");
    events->add_listener("goto-settings-page", [this](const value& data) { state_machine.handle_event(ui_event::UI_EVENT_GO_TO_SETTINGS_PAGE); });

    state_machine.handle_event(ui_event::UI_EVENT_GO_TO_PROJECT_PAGE);
  }

  void server_ui::render() {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer is null in server UI");
    renderer_ptr->begin_ui_frame();
    render_all();
    renderer_ptr->end_ui_frame();
  }

  void server_ui::render_all() {
    switch (state_machine.get_current_state()) {
      case UI_STATE_PROJECT_PAGE: render_all_project_page(); break;
      case UI_STATE_CREATE_PROJECT_PAGE: render_all_create_project_page(); break;
      case UI_STATE_SETTINGS_PAGE: render_all_settings_page(); break;
      default: break;
    }
  }

  void server_ui::render_all_project_page() {
    project_win->render();
  }

  void server_ui::render_all_create_project_page() {
    create_project_win->render();
  }

  void server_ui::render_all_settings_page() {
  }

  void server_ui::validate_object_and_render_project(const json::json& json_obj) {
    bool has_name = json_obj.contains("name") && json_obj["name"].is_string();
    bool has_project_file = json_obj.contains("project-file") && json_obj["project-file"].is_string();
    bool has_working_directory = json_obj.contains("working-directory") && json_obj["working-directory"].is_string();

    if (!(has_name && has_project_file && has_working_directory)) {
      auto format_and_append_field_str = [](const std::string& field_name, bool has_field, const json::json& obj, std::string& out_str) {
        if (!has_field) {
          out_str += "\n - missing [" + field_name + "]";
        } else {
          out_str += "\n - " + field_name + ": " + obj[field_name].get<std::string>();
        }
      };

      std::string error_msg = "Corrupt project cache entry :";
      format_and_append_field_str("name", has_name, json_obj, error_msg);
      format_and_append_field_str("project-file", has_project_file, json_obj, error_msg);
      format_and_append_field_str("working-directory", has_working_directory, json_obj, error_msg);

      scoped_color error(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
      ImGui::TextWrapped("%s", error_msg.c_str());
      return;
    } else {
      std::string project_name = json_obj["name"].get<std::string>();
      std::string project_path = json_obj["project-file"].get<std::string>();
      std::string working_directory = json_obj["working-directory"].get<std::string>();

      if (ImGui::TreeNode(project_name.c_str())) {
        ImGui::Text("Path: %s", project_path.c_str());
        ImGui::Text("Working Directory: %s", working_directory.c_str());

        if (ImGui::Button(("Open " + project_name).c_str())) {
          CORE_LOG_INFO("Request to open project '{}' at path '{}'", project_name, project_path);
        }

        ImGui::TreePop();
      }
    }
  }

}  // namespace other