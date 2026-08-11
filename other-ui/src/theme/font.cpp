/**
 * \file theme/font.cpp
 **/
#include "theme/font.hpp"

namespace other {

  ImFont* font_set::get(font_role role) const {
    OTHER_ASSERT((size_t)role < (size_t)font_role::NUM_ROLES, "Invalid font role.");
    return fonts[(size_t)role];
  }

  bool font_set::needs_rebuild() const {
    ///    \todo: ...
    return false;
  }

}  // namespace other