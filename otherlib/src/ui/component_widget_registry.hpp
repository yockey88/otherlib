/**
 * \file ui/component_widget_registry.hpp
 **/
#ifndef OTHERLIB_UI_COMPONENT_WIDGET_REGISTRY_HPP
#define OTHERLIB_UI_COMPONENT_WIDGET_REGISTRY_HPP

#include "ui/field_ui.hpp"

namespace other {
  namespace ui {

    using component_draw_fn = std::function<bool(const std::string_view label, scene*, scene_object*, const field_context&)>;

    class component_widget_registry {
     public:
      void register_widget(natural_t component_type_id, const std::string_view component_name, component_draw_fn fn, bool override_existing = false);
      bool draw(natural_t component_type_id, const std::string_view label, scene* s, scene_object* o, const field_context& ctx) const;

      inline bool has(natural_t component_type_id) const { return drawers.contains(component_type_id); }

     private:
      struct entry {
        std::string component_name;
        component_draw_fn fn;
      };
      std::unordered_map<natural_t, entry> drawers;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COMPONENT_WIDGET_REGISTRY_HPP
