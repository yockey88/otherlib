/**
 * \file ui/property_inspector_node.hpp
 **/
#ifndef OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP
#define OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP

#include <imgui/ImReflect.hpp>

#include "renderer/ui/ui_node.hpp"

#include "object/component.hpp"

namespace other {

  class driver;
  class scene;
  struct scene_object;

  namespace ui {

    template <typename T>
    struct type_settings {
      static ImReflect::ImSettings get() {
        return ImReflect::ImSettings();
      }
    };

    class property_inspector_node : public ui_node {
     public:
      property_inspector_node(ui_window* window, driver* drvr);
      virtual ~property_inspector_node() = default;

     private:
      driver* driver_ptr = nullptr;

      bool multi_selection_enabled = false;
      std::deque<natural_t> selected_object_ids;

      void handle_object_selection(natural_t object_id);

      template <typename T>
      using on_component_modified_fn = void (*)(T* component, scene_object* object, scene* active_scene, driver* drvr);

      template <typename T>
      void draw_component_section(const std::string_view component_name, scene* active_scene, scene_object* object, on_component_modified_fn<T> on_modified = nullptr);

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP