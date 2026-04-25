/**
 * \file ui/menu-bar/menu_item.cpp
 **/
#include "ui/menu-bar/menu_item.hpp"

#include <imgui/imgui.h>

namespace other {
  namespace ui {

    void menu_item::render() {
      if (ImGui::BeginMenu(name.c_str())) {
        if (!sub_items.empty()) {
          for (auto& sub_item : sub_items) {
            sub_item.render();
          }
        } else {
          if (ImGui::MenuItem(name.c_str())) {
            // Handle menu item click event here
          }
        }
        ImGui::EndMenu();
      }
    }

  }  // namespace ui
}  // namespace other