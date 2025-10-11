/**
 * \file server-dev/server-ui/project-creator.cpp
 **/
#include "project-creator.hpp"

#include <imgui/imgui.h>
#include <nfd/nfd.h>
#include <nfd_common.h>

#include "core/defines.hpp"

#include "renderer/ui/nodes/hbox_node.hpp"
#include "renderer/ui/ui_node.hpp"

namespace other {
  namespace detail {

    struct create_project_node : public ui_node {
      create_project_node(ui_window* parent, project_creator& creator)
          : ui_node(parent, "Create Project"), creator(creator) {}
      virtual ~create_project_node() = default;

      void render_node() override;
      void select_path();

      project_creator& creator;
    };

    struct rect_node : public ui_node {
      rect_node(ui_window* parent, const std::string& name)
          : ui_node(parent, name) {}
      virtual ~rect_node() = default;

      void render_node() override {
        ImGui::Text("Rect Node");
      }
    };

  }  // namespace detail

  project_creator::project_creator(event_system& events)
      : ui_window(events, "Create New Project") {
    context.project_name = "";
    context.project_path = "";
    context.working_directory = filepath(get_system_default_working_directory());

    // add_node(make_scope<hbox_node>(this, "hbox-1"));
    add_node(make_scope<detail::create_project_node>(this, *this));  //, "hbox-1");
    // add_node(make_scope<detail::rect_node>(this, "rect-1"), "hbox-1");
  }

  namespace detail {

    void create_project_node::render_node() {
      if (ImGui::Button("Cancel")) {
        trigger_event("goto-project-page");
        return;
      }

      /// two subwindows
      ///   1- environment config: options for other-env .toml that sets up environment for other application
      ///   2- project config: options for project file (.otherproj) that sets up project-specific settings
      ImGui::Text("Create New Project");
      ImGui::Separator();
      ImGui::Spacing();
      ImGui::Spacing();

      ImGui::InputText("Project Name", creator.context.project_name_buffer.data(), creator.context.kMaxProjectNameLength);
      creator.context.project_name = std::string(creator.context.project_name_buffer.data());

      // std::string disp_name = creator.context.project_name.empty() ? "Untitled" : creator.context.project_name;
      // std::string disp_path = (creator.context.working_directory / disp_name / (disp_name + ".otherproj")).string();
      // std::string disp_working_dir = (creator.context.working_directory / disp_name).string();

      // ImGui::Text("Project Name: %s", disp_name.c_str());
      // ImGui::Text("Project File: %s", disp_path.c_str());
      // ImGui::Text("Working Directory: %s", disp_working_dir.c_str());

      // if (ImGui::Button("Select Location")) {
      //   nfdchar_t* outPath = nullptr;
      //   nfdresult_t result = NFD_PickFolder(nullptr, &outPath);

      //   std::string selected_path;
      //   if (result == NFD_OKAY) {
      //     selected_path = std::string(outPath);
      //     NFDi_Free(outPath);

      //     filepath project_directory = selected_path / filepath(creator.context.project_name);
      //     if (!creator.context.project_name.empty()) {
      //       creator.context.project_path = project_directory / filepath(creator.context.project_name + ".otherproj");

      //       if (std::filesystem::exists(project_directory)) {
      //         CORE_LOG_WARN("Project directory already exists: {}", project_directory.string());
      //       } else {
      //         std::filesystem::create_directories(project_directory);
      //       }
      //     }
      //     creator.context.working_directory = project_directory;

      //     CORE_LOG_INFO("User selected project path: {}", selected_path);
      //   } else if (result == NFD_CANCEL) {
      //   } else {
      //     CORE_LOG_ERROR("Error selecting folder: {}", NFD_GetError());
      //   }
      // }

      // ImGui::InputText("Project Name", creator.context.project_name_buffer.data(), creator.context.kMaxProjectNameLength);
      // creator.context.project_name = std::string(creator.context.project_name_buffer.data());
    }

  }  // namespace detail
}  // namespace other