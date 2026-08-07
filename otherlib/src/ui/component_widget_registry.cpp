/**
 * \file ui/component_widget_registry.cpp
 **/
#include "ui/component_widget_registry.hpp"

#include "core/profiler.hpp"

#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"

namespace other {
  namespace ui {

    void component_widget_registry::register_widget(natural_t component_type_id, const std::string_view component_name, component_draw_fn fn, bool override_existing) {
      OTHER_ASSERT(fn != nullptr, "Null component_draw_fn for component '{}'", component_name);
      PROFILE_SECTION("component_widget_registry::register_widget");
      auto it = drawers.find(component_type_id);
      if (it != drawers.end()) {
        OTHER_ASSERT(override_existing, "Duplicate component drawer for '{}' (pass override_existing=true to replace)", component_name);
        it->second = entry{ std::string{ component_name }, std::move(fn) };
      } else {
        drawers.emplace(component_type_id, entry{ std::string{ component_name }, std::move(fn) });
      }
    }

    bool component_widget_registry::draw(natural_t component_type_id, const std::string_view label, scene* s, scene_object* o, const field_context& ctx) const {
      OTHER_ASSERT(s != nullptr, "Scene is null drawing component '{}'", label);
      OTHER_ASSERT(o != nullptr, "Scene object is null drawing component '{}'", label);
      PROFILE_SECTION("component_widget_registry::draw");
      auto it = drawers.find(component_type_id);
      if (it == drawers.end()) {
        inspector::property_display(label, "<no drawer registered>", colors::kTextDisabled);
        return false;
      }
      return it->second.fn(label, s, o, ctx);
    }

  }  // namespace ui
}  // namespace other