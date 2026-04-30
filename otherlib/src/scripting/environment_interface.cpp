/**
 * \file scripting/environment_interface.cpp
 **/
#include "scripting/environment_interface.hpp"

namespace other {

  environment_interface get_menu_item_interface() {
    return {
      .name = "MenuItemInterface",
      .actions = {
        action("OnClick", "Defines the callback for when the menu item is clicked"),
        action("OnHover", "Defines the callback for when the menu item is hovered"),
        action("OnOpenSubMenu", "Defines the callback for when the sub menu of this menu item is opened"),
        action("OnCloseSubMenu", "Defines the callback for when the sub menu of this menu item is closed"),
        action("OnAddSubItem", "Defines the callback for when a sub menu item is added to this menu item"),
        action("OnRemoveSubItem", "Defines the callback for when a sub menu item is removed from this menu item"),
      }
    };
  }

  environment_interface get_menu_bar_interface() {
    return {
      .name = "MenuBarInterface",
      .actions = {
        action("Render", "Renders the menu bar"),
        action("OnAddMenuItem", "Adds a menu item to the menu bar"),
        action("OnRemoveMenuItem", "Removes a menu item from the menu bar"),
      }
    };
  }

}  // namespace other