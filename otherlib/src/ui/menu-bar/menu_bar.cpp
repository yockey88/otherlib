/**
 * \file ui/menu-bar/menu_bar.cpp
 **/
#include "ui/menu-bar/menu_bar.hpp"

#include <imgui/imgui.h>

namespace other {
  namespace ui {

    void menu_bar::render(std::span<menu> menus) {
      bool open = false;
      if (main_menu_bar) {
        open = ImGui::BeginMainMenuBar();
      } else {
        open = ImGui::BeginMenuBar();
      }

      if (open) {
        for (auto& menu : menus) {
          menu.render();
        }
      }

      if (open && main_menu_bar) {
        ImGui::EndMainMenuBar();
      } else if (open) {
        ImGui::EndMenuBar();
      }
    }

  }  // namespace ui
}  // namespace other