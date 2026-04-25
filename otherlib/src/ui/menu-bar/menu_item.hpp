/**
 * \file ui/menu-bar/menu_item.hpp
 **/
#ifndef OTHERLIB_UI_MENU_BAR_MENU_ITEM_HPP
#define OTHERLIB_UI_MENU_BAR_MENU_ITEM_HPP

#include <string>
#include <vector>

namespace other {
  namespace ui {

    struct menu_item {
      std::string name;
      std::vector<menu_item> sub_items;

      void render();
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_MENU_BAR_MENU_ITEM_HPP