/**
 * \file ui/property_inspector_node.cpp
 **/
#include "ui/property_inspector_node.hpp"

#include <string>

#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "object/animation_controller.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "ui/colors.hpp"
#include "ui/component_widget.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/ui_widgets.hpp"

#include "imgui.h"
#include "inspector_widgets.hpp"

IMGUI_REFLECT(glm::vec3, x, y, z);
IMGUI_REFLECT(glm::quat, w, x, y, z);

IMGUI_REFLECT(other::gpu::graphics_material, diffuse_color, diffuse_reflectivity, specular_color, specular_reflectivity, emissivity, transparency, shininess, padding);

IMGUI_REFLECT(other::scene_object, id, registry_id, name, visible);
IMGUI_REFLECT(other::transform, local_position, local_rotation_quat, local_scale);
IMGUI_REFLECT(other::script_component, script_object_id);
IMGUI_REFLECT(other::render_component, material, visible, animated);
IMGUI_REFLECT(other::model, name, submesh_indices);
IMGUI_REFLECT(other::physics_component, body, shape);
IMGUI_REFLECT(other::physics_body_settings, body_type, mass);
IMGUI_REFLECT(other::light_component, point_lights, directional_lights);  //, light_type, color, intensity, range, inner_cone_angle, outer_cone_angle);
IMGUI_REFLECT(other::gpu::point_light, light_position, color);
IMGUI_REFLECT(other::gpu::directional_light, direction, color);

IMGUI_REFLECT(other::orthonormal_basis, i, j, k);
IMGUI_REFLECT(other::camera, position, direction, euler_angles, world_up, basis);
IMGUI_REFLECT(other::camera_component, camera);

namespace other {

  namespace ui {

    property_inspector_node::property_inspector_node(ui_window* window, driver* drvr)
        : ui_node(window, "Property Inspector"), driver_ptr(drvr) {
      events().add_listener("ui.scene-hierarchy.object-selected", [this](const value& data) {
        if (data.type() != value_type::UINT64) {
          CORE_LOG_ERROR("Invalid data type for object-selected event. Expected uint64.");
          return;
        }

        natural_t object_id = data;
        handle_object_selection(object_id);
      });
    }

    void property_inspector_node::handle_object_selection(natural_t object_id) {
      CORE_LOG_DEBUG("Handling object selection for ID {}", object_id);
      if (object_id == 0) {
        /// unselect anything selected
        selected_object_ids.clear();
        return;
      }

      if (multi_selection_enabled) {
        auto it = std::ranges::find(selected_object_ids, object_id);
        if (it != selected_object_ids.end()) {
          return;
        }
      }
      /// if multi-select is off, clear previous selection if any
      else if (selected_object_ids.size() > 0) {
        selected_object_ids.clear();
      }
      CORE_LOG_DEBUG("Selecting object ID {}", object_id);
      selected_object_ids.push_back(object_id);
    }

    template <typename T>
    void property_inspector_node::draw_component_section(const std::string_view component_name, scene* active_scene, scene_object* object) {
      if (!active_scene->has_component<T>(object)) {
        return;
      }

      T* comp = active_scene->get_component<T>(object);
      OTHER_ASSERT(comp != nullptr, "Component of type '{}' not found on object with ID {}", component_name, object->id);

      const auto tag = comp->get_id();
      inspector::component_section_flags section_flags{};
      if constexpr (std::is_same_v<T, transform>) {
        section_flags.removable = false;
      }

      bool remove_requested = false;
      bool is_open = inspector::begin_component_section(component_name, tag, section_flags, &remove_requested);

      if (is_open) {
        component_widget<T>{}(component_name, *comp, active_scene, object);
      }

      inspector::end_component_section();

      /// \todo flesh this out more, this could be it but it may be more complicated
      if (remove_requested) {
        // active_scene->remove_component<T>(object);
      }
    }

    void property_inspector_node::on_render_node_body() {
      if (!ImGui::BeginChild("##property-inspector")) {
        ImGui::EndChild();
        return;
      }

      if (selected_object_ids.empty()) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No object selected.");
      } else if (selected_object_ids.size() > 1) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextBright));
        ImGui::Text("Multiple objects selected (%zu).", selected_object_ids.size());
      } else {
        natural_t obj_id = selected_object_ids.front();

        auto* active_scene = driver_ptr->get_active_scene();
        OTHER_ASSERT(active_scene != nullptr, "Active scene is null, cannot render properties.");

        scene_object& obj = active_scene->get_object(obj_id);

        char name_buf[256];
        std::strncpy(name_buf, obj.name.c_str(), sizeof(name_buf));
        name_buf[sizeof(name_buf) - 1] = '\0';

        if (inspector::draw_object_header_editable(name_buf, sizeof(name_buf), obj.id, colors::scene_object::kSignature)) {
          obj.name = std::string(name_buf);
        }
        // ImGui::Separator();

        /// components
        draw_component_section<transform>("Transform", active_scene, &obj);
        draw_component_section<script_component>("Scripts", active_scene, &obj);
        draw_component_section<render_component>("Graphics Object", active_scene, &obj);
        draw_component_section<camera_component>("Camera", active_scene, &obj);
        draw_component_section<physics_component>("Physics Body", active_scene, &obj);
        draw_component_section<light_component>("Lights", active_scene, &obj);
        draw_component_section<animation_controller>("Animation Controller", active_scene, &obj);

        if (inspector::draw_add_component_button()) {
          /// \todo open component picker popup
          ImGui::OpenPopup("##add_component_popup");
        }

        /// \todo component picker popup
        if (ImGui::BeginPopup("##add_component_popup")) {
          scoped_color text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kText));
          ImGui::Text("Add Component...");
          ImGui::Separator();
          /// list available component types here
          ImGui::EndPopup();
        }
      }

      ImGui::EndChild();
    }

  }  // namespace ui
}  // namespace other