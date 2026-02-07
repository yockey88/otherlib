/**
 * \file editor_ui.cpp
 **/
#include "editor_ui.hpp"

#include <imgui/imgui.h>

#include "script/scripting_environment.hpp"

namespace other {

  void editor_ui::initialize() {
    console_window_ptr = make_scope<ui::console_window>(*event_system, driver_ptr);
  }

  void editor_ui::render() {
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project")) {
          event_system->trigger_event("editor:main-menu:file:new-project");
        }
        if (ImGui::MenuItem("Open Project")) {
          event_system->trigger_event("editor:main-menu:file:open-project");
        }
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if (ui_state_machine.get_current_state() == editor_ui_state::EDITOR_UI_STATE_CONSOLE) {
      console_window_ptr->render();
    }
  }

  void editor_ui::shutdown() {
    console_window_ptr = nullptr;
  }

}  // namespace other