/**
 * \file ui/menu-bar/menu_item.cpp
 **/
#include "ui/menu-bar/menu_item.hpp"

#include <imgui/imgui.h>

namespace other {
  namespace ui {

    void menu_item::render() {
      if (ImGui::MenuItem(name.c_str())) {
        if (action && action->has_callback()) {
          action->execute<>();
        }
      }
    }

  }  // namespace ui
}  // namespace other