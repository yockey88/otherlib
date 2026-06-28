/**
 * \file renderer/ui/ui_interactable.hpp
 **/
#ifndef OTHERLIB_RENDERER_UI_UI_INTERACTABLE_HPP
#define OTHERLIB_RENDERER_UI_UI_INTERACTABLE_HPP

namespace other {

  struct ui_interactable {
    virtual ~ui_interactable() = default;

    bool hovered = false;
    bool clicked = false;
    bool right_clicked = false;
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_UI_UI_INTERACTABLE_HPP