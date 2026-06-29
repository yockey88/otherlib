/**
 * \file ui/menu-bar/menu.cpp
 **/
#include "ui/menu-bar/menu.hpp"

namespace other {
  namespace ui {

    void menu::render() {
      if (ImGui::BeginMenu(name.c_str())) {
        if (dynamic_sub_menus != nullptr) {
          dynamic_sub_menus();
        }

        for (auto& sub_menu : sub_menus) {
          sub_menu.render();
        }

        if (dynamic_items != nullptr) {
          dynamic_items();
        }

        for (auto& item : items) {
          item.render();
        }

        ImGui::EndMenu();
      }
    }

  }  // namespace ui
}  // namespace other