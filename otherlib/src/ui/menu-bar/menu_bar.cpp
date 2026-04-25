/**
 * \file ui/menu-bar/menu_bar.cpp
 **/
#include "ui/menu-bar/menu_bar.hpp"

#include <imgui/imgui.h>

namespace other {
  namespace ui {

    void menu_bar::render(std::span<menu_item> items) {
      if (ImGui::BeginMainMenuBar()) {
        for (auto& item : items) {
          item.render();
        }
        ImGui::EndMainMenuBar();
      }
    }

  }  // namespace ui
}  // namespace other