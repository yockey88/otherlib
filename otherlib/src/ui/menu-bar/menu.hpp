/**
 * \file ui/menu-bar/menu.hpp
 **/
#ifndef OTHERLIB_UI_MENU_BAR_MENU_HPP
#define OTHERLIB_UI_MENU_BAR_MENU_HPP

#include <string>

#include "ui/menu-bar/menu_item.hpp"

namespace other {
  namespace ui {

    struct menu {
      std::string name;
      std::vector<menu> sub_menus;
      std::vector<menu_item> items;

      std::function<void()> dynamic_sub_menus = nullptr;
      std::function<void()> dynamic_items = nullptr;

      void render();
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_MENU_BAR_MENU_HPP