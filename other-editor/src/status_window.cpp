/**
 * \file status_window.cpp
 **/
#include "status_window.hpp"

#include "editor_driver.hpp"

namespace other {
  namespace ui {

    void status_window::on_render_body() {
      OTHER_ASSERT(driver_ptr != nullptr, "Status window has null driver pointer");
      bool scene_loaded = driver_ptr->get_active_scene() != nullptr;

      if (scene_loaded) {
        ImGui::Text("Scene: %s", driver_ptr->get_active_scene()->name.c_str());
      } else {
        ImGui::Text("No scene loaded.");
        if (ImGui::Button("Load Empty Scene")) {
          driver_ptr->new_blank_scene("Empty Scene");
        }
      }
    }

  }  // namespace ui
}  // namespace other