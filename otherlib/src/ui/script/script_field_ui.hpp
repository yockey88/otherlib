/**
 * \file ui/script/script_field_ui.hpp
 **/
#ifndef OTHERLIB_UI_SCRIPT_FIELD_UI_HPP
#define OTHERLIB_UI_SCRIPT_FIELD_UI_HPP

#include <glm/vec2.hpp>

#include "dotnet/behavior_descriptor.hpp"

#include "asset/asset.hpp"

namespace other {

  struct scene_object;

  class asset_handler;
  class scene;
  class driver;

  namespace ui {

    struct field_flags {
      bool read_only = false;
      bool is_color = false;
      bool has_range = false;

      glm::vec2 range = glm::vec2(0.0f, 1.0f);
      opt<float> speed;
      asset::type asset_type = asset::EMPTY;

      std::string display_name;
      std::string tooltip;
    };

    struct field_context {
      scene* scene_ptr = nullptr;
      scene_object* scene_object_ptr = nullptr;
      asset_handler* asset_handler_ptr = nullptr;
      driver* driver_ptr = nullptr;
      field_flags flags = {};
    };

    field_flags field_flags_from_behavior(const behavior_field_descriptor& f);

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCRIPT_FIELD_UI_HPP