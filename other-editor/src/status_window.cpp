/**
 * \file status_window.cpp
 **/
#include "status_window.hpp"

#include "driver/systems/scene_system.hpp"

#include "editor_driver.hpp"

namespace other {
  namespace ui {

    void status_window::on_render_body() {
      OTHER_ASSERT(driver_ptr != nullptr, "Status window has null driver pointer");

      auto& kernel = driver_ptr->get_kernel();
      auto& scenes = kernel.get_core_system<scene_system>();
      bool scene_loaded = scenes.get_active_scene() != nullptr;

      if (scene_loaded) {
        ImGui::Text("Scene: %s", scenes.get_active_scene()->name.c_str());
      } else {
        ImGui::Text("No scene loaded.");
        if (ImGui::Button("Load Empty Scene")) {
          scenes.new_blank_scene("Empty Scene");
        }
      }
    }

  }  // namespace ui
}  // namespace other