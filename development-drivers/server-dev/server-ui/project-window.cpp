/**
 * \file server-dev/server-ui/project-window.cpp
 **/
#include "project-window.hpp"

#include "core/logger.hpp"

#include "renderer/ui/ui_helpers.hpp"
#include "renderer/ui/ui_node.hpp"

#include "imgui.h"

namespace other {
  namespace {

    struct project_list_node : public ui_node {
      project_list_node(ui_window* parent, json::json& project_cache)
          : ui_node(parent, "project-list"), project_cache(project_cache) {
      }
      virtual ~project_list_node() = default;

      void render_node() override {
        if (ImGui::Button("Create New Project")) {
          trigger_event("create-project");
          return;
        }

        if (project_cache.contains("projects") && project_cache["projects"].is_array()) {
          for (const auto& json_obj : project_cache["projects"]) {
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
        }
      }

      json::json& project_cache;
    };

  }  // namespace

  project_window::project_window(event_system& events, json::json& project_cache)
      : ui_window(events, "Projects"), project_cache(project_cache) {
    add_node(make_scope<project_list_node>(this, project_cache));
  }

}  // namespace other