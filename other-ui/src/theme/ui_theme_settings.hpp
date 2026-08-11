/**
 * \file theme/ui_theme_settings.hpp
 **/
#ifndef OTHER_UI_THEME_UI_THEME_SETTINGS_HPP
#define OTHER_UI_THEME_UI_THEME_SETTINGS_HPP

#include <glm/glm.hpp>

struct ImGuiStyle;

namespace other {

  struct ui_theme_settings {
    float density = 1.f;
    float rounding = 2.f;
    float border_size = 1.f;
    glm::vec2 item_spacing = { 8.f, 4.f };
    glm::vec2 frame_padding = { 6.f, 3.f };
    glm::vec2 cell_padding = { 4.f, 2.f };
    float indent_spacing = 18.f;
    float scrollbar_size = 12.f;
    float grab_min_size = 10.f;

    void apply(ImGuiStyle* style) const;
  };

}  // namespace other

#endif  // OTHER_UI_THEME_UI_THEME_SETTINGS_HPP