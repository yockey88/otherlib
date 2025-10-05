/**
 * \file server-dev/server-ui/server-ui.cpp
 **/
#include "server-ui.hpp"

#include <string>

#include <imgui/imgui.h>

#include "renderer/ui/ui_helpers.hpp"

#include "project-window.hpp"

namespace other {
  namespace detail {

    struct ui_window_end_helper {
      ~ui_window_end_helper() { ImGui::End(); }
    };

#define START_WINDOW(name)                                         \
  detail::ui_window_end_helper ___ui_window_end_helper_instance{}; \
  if (!ImGui::Begin(name)) {                                       \
    return;                                                        \
  }

  }  // namespace detail

  server_ui::server_ui(scope<renderer>& renderer_ptr, json::json& project_cache)
      : renderer_ptr(renderer_ptr), project_cache(project_cache) {
    project_win = make_scope<project_window>(project_cache);
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
    OTHER_ASSERT(active_project.has_value(), "No active project context in create project page");
    START_WINDOW("Create New Project");

    if (ImGui::Button("Cancel")) {
      state_machine.handle_event(ui_event::UI_EVENT_GO_TO_PROJECT_PAGE);
      return;
    }

    /**
     * - Project Name
     * - Project File (file dialog)
     * - Working Directory
     */

    ImGui::Text("Project Name: %s", active_project->project_name.c_str());
    ImGui::Text("Project File: %s", active_project->project_path.string().c_str());
    ImGui::Text("Working Directory: %s", active_project->working_directory.string().c_str());

    if (ImGui::Button("Select Location")) {
      nfdchar_t* outPath = nullptr;
      nfdresult_t result = NFD_PickFolder(nullptr, &outPath);
      std::string selected_path;
      if (result == NFD_OKAY) {
        selected_path = std::string(outPath);
        // NFD_FreePath(outPath);
        active_project->project_path = filepath(selected_path) / filepath(active_project->project_name);
        CORE_LOG_INFO("User selected project path: {}", selected_path);
      } else if (result == NFD_CANCEL) {
        // CORE_LOG_INFO("User canceled folder selection");
      } else {
        // CORE_LOG_ERROR("Error selecting folder: {}", NFD_GetError());
      }
    }
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