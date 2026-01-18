/**
 * \file ui/interactable_item.hpp
 **/
#ifndef OTHERLIB_UI_INTERACTABLE_ITEM_HPP
#define OTHERLIB_UI_INTERACTABLE_ITEM_HPP

#include <glm/glm.hpp>

namespace other {
  namespace ui {

    struct interactable {
      struct {
        bool hovered = false;
        bool clicked = false;
        bool right_clicked = false;
        bool dragging = false;
      } state;

      glm::vec2 position = glm::vec2(0, 0);
      glm::vec2 size = glm::vec2(0, 0);

      virtual ~interactable() = default;
    };

  }  // namespace ui

}  // namespace other

#endif  // OTHERLIB_UI_INTERACTABLE_ITEM_HPP