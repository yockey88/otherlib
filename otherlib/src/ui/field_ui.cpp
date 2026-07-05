/**
 * \file ui/script/script_field_ui.cpp
 **/
#include "ui/field_ui.hpp"

namespace other {
  namespace ui {

    field_flags field_flags_from_behavior(const behavior_field_descriptor& f) {
      field_flags flags;
      flags.read_only = has_flag(f.flags, behavior_display_flags::READ_ONLY);
      flags.is_color = has_flag(f.flags, behavior_display_flags::COLOR_FIELD);
      flags.has_range = has_flag(f.flags, behavior_display_flags::HAS_RANGE);
      flags.range = glm::vec2(f.range_min, f.range_max);
      flags.speed = f.speed;
      flags.display_name = f.display_name;
      flags.tooltip = f.tooltip;
      return flags;
    }

  }  // namespace ui
}  // namespace other