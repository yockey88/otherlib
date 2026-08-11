/**
 * \file theme/font.hpp
 **/
#ifndef OTHER_UI_THEME_FONT_HPP
#define OTHER_UI_THEME_FONT_HPP

#include <array>
#include <string>

struct ImFont;

namespace other {

  enum class font_role {
    DEFAULT,
    MONOSPACE,
    HEADING,
    SMALL,
    ICONS,
    NUM_ROLES,
  };

  struct font_spec {
    std::string file;
    float size = 16.f;
  };

  class font_set {
   public:
    ImFont* get(font_role role) const;
    bool needs_rebuild() const;

   private:
    std::array<font_spec, (size_t)font_role::NUM_ROLES> specs;
    std::array<ImFont*, (size_t)font_role::NUM_ROLES> fonts;
  };

}  // namespace other

#endif  // OTHER_UI_THEME_FONT_HPP