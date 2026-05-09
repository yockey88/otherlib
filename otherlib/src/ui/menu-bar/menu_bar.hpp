/**
 * \file ui/menu-bar/menu_bar.hpp
 **/
#ifndef OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP
#define OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP

#include <span>

#include "ui/menu-bar/menu.hpp"

namespace other {
  namespace ui {

    struct menu_bar {
      bool main_menu_bar = false;

      void render(std::span<menu> menus);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP