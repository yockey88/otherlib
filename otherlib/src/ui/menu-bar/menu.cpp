/**
 * \file ui/menu-bar/menu.cpp
 **/
#include "ui/menu-bar/menu.hpp"

namespace other {
  namespace ui {

    void menu::render() {
      if (ImGui::BeginMenu(name.c_str())) {
        for (auto& sub_menu : sub_menus) {
          sub_menu.render();
        }
        for (auto& item : items) {
          item.render();
        }
        ImGui::EndMenu();
      }
    }

  }  // namespace ui
}  // namespace other