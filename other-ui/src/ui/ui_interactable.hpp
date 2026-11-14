/**
 * \file ui/ui_interactable.hpp
 **/
#ifndef OTHER_UI_UI_UI_INTERACTABLE_HPP
#define OTHER_UI_UI_UI_INTERACTABLE_HPP

namespace other {

  struct ui_interactable {
    virtual ~ui_interactable() = default;

    bool hovered = false;
    bool clicked = false;
    bool right_clicked = false;
  };

}  // namespace other

#endif  // OTHER_UI_UI_UI_INTERACTABLE_HPP