/**
 * \file ui/object-editor/property_inspector_node.hpp
 **/
#ifndef OTHERLIB_UI_OBJECT_EDITOR_PROPERTY_INSPECTOR_NODE_HPP
#define OTHERLIB_UI_OBJECT_EDITOR_PROPERTY_INSPECTOR_NODE_HPP

#include <imgui/ImReflect.hpp>

#include "ui/ui_node.hpp"

#include "editor_context.hpp"

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
      property_inspector_node(editor_context& context, ui_window* window, driver* drvr);
      virtual ~property_inspector_node() = default;

     private:
      editor_context& context;
      driver* driver_ptr = nullptr;

      template <typename T>
      using on_component_modified_fn = void (*)(T* component, scene_object* object, scene* active_scene, driver* drvr);

      template <typename T>
      void draw_component_section(const std::string_view component_name, const glm::vec4& color, scene* active_scene, scene_object* object, on_component_modified_fn<T> on_modified = nullptr);

      template <typename T>
      bool draw_component_selector(const std::string_view component_name, scene* active_scene, scene_object* object);

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_OBJECT_EDITOR_PROPERTY_INSPECTOR_NODE_HPP