/**
 * \file driver/detail/component_ui_widgets.hpp
 **/
#ifndef OTHERLIB_DRIVER_DETAIL_COMPONENT_UI_WIDGETS_HPP
#define OTHERLIB_DRIVER_DETAIL_COMPONENT_UI_WIDGETS_HPP

#include "scene/scene.hpp"

#include "driver/systems/scene_system.hpp"
#include "ui/component_widget.hpp"
#include "ui/component_widget_registry.hpp"

namespace other {
  namespace detail {

    template <typename T>
    ui::component_draw_fn get_default_component_widget_fn(const std::string_view label, scene* s, scene_object* o, const ui::field_context& ctx) {
      OTHER_ASSERT(s != nullptr, "Scene pointer is null in default component widget function.");
      OTHER_ASSERT(o != nullptr, "Scene object pointer is null in default component widget function.");
      T* comp = s->template get_component<T>(o);
      OTHER_ASSERT(comp != nullptr, "Component of type '{}' not found on object with ID {} in default component widget function.", typeid(T).name(), o->id);
      return ui::component_widget<T>{}(label, *comp, s, o, ctx.asset_handler_ptr, ctx.driver_ptr);
    }

    template <typename T>
      requires std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>
    natural_t register_editor_component_type(driver* drvr, const std::string_view name, const glm::vec4& color, component_type_flags flags = {}) {
      OTHER_ASSERT(drvr != nullptr, "Driver is null registering editor component '{}'", name);
      auto& scenes = drvr->get_kernel().get_core_system<scene_system>();
      const natural_t type_id = scenes.get_component_registry().register_component_type<T>(name, color, flags);
      drvr->get_ui()->get_component_widget_registry().register_widget(type_id, name, get_default_component_widget_fn<T>);
      return type_id;
    }

  }  // namespace detail
}  // namespace other

#endif  // OTHERLIB_DRIVER_DETAIL_COMPONENT_UI_WIDGETS_HPP