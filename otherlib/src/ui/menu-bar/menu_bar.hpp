/**
 * \file ui/menu-bar/menu_bar.hpp
 **/
#ifndef OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP
#define OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP

#include <span>

#include "ui/menu-bar/menu_item.hpp"

namespace other {
  namespace ui {

    class menu_bar {
     public:
      void render(std::span<menu_item> items);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_MENU_BAR_MENU_BAR_HPP