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
#include "object/script_component.hpp"

#include "driver/driver.hpp"
#include "ui/colors.hpp"
#include "ui/component_widget.hpp"
#include "ui/ui_widgets.hpp"

#include "imgui.h"

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

  template <>
  struct property_ui<glm::vec2> {
    bool operator()(const std::string& name, glm::vec2& value, scene* active_scene, scene_object* object) {
      ImGui::PushID(name.c_str());
      bool changed = ui::edit_vec2(name, value);
      ImGui::PopID();
      return changed;
    }
  };
  template <>
  struct property_ui<glm::vec3> {
    bool operator()(const std::string& name, glm::vec3& value, scene* active_scene, scene_object* object) {
      ImGui::PushID(name.c_str());
      bool changed = ui::edit_vec3(name, value);
      ImGui::PopID();
      return changed;
    }
  };
  template <>
  struct property_ui<glm::vec4> {
    bool operator()(const std::string& name, glm::vec4& value, scene* active_scene, scene_object* object) {
      ImGui::PushID(name.c_str());
      bool changed = ui::edit_vec4(name, value);
      ImGui::PopID();
      return changed;
    }
  };

  template <>
  struct property_ui<glm::quat> {
    bool operator()(const std::string& name, glm::quat& value, scene* active_scene, scene_object* object) {
      ImGui::PushID(name.c_str());
      bool changed = ui::edit_quat(name, value);
      ImGui::PopID();
      return changed;
    }
  };

  template <>
  struct property_ui<orthonormal_basis> {
    bool operator()(const std::string& name, orthonormal_basis& value, scene* active_scene, scene_object* object) {
      ImGui::PushID(name.c_str());
      ui::draw_mat3(name, glm::mat3(value.to_matrix()));
      ImGui::PopID();
      return false;
    }
  };

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
      selected_object_ids.push_back(object_id);
    }

    template <typename T>
    void draw_component(const std::string_view component_name, scene* active_scene, scene_object* object) {
      if (!active_scene->has_component<T>(object)) {
        return;
      }

      bool component_node_open = ImGui::TreeNodeEx(component_name.data(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
      if (!component_node_open) {
        return;
      }
      bool modified = false;
      {
        T& comp = *active_scene->get_component<T>(object);
        reflection_data& type_data = type_data_handler<T>::get_reflection_data(comp);

        ImGui::Separator();
        {
          scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextBright));
          ImGui::Text("%s Component [%s]", component_name.data(), type_data.type_name.c_str());
        }
        ImGui::Separator();
        modified = component_widget<T>{}(component_name, comp, active_scene, object);
      }

      ImGui::TreePop();
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

        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextBright));
        ImGui::Text("Properties for Object:\n  - %s (ID: %llu)", obj.name.c_str(), obj.id);

        /// identifiers
        ImGui::Separator();
        ImGui::Text("ID: %llu", obj.id);
        ImGui::Text("Name:");

        char name_buf[256];
        std::strncpy(name_buf, obj.name.c_str(), sizeof(name_buf));
        if (ImGui::InputText("##object-name", name_buf, sizeof(name_buf))) {
          obj.name = std::string(name_buf);
        }
        ImGui::Separator();

        /// components
        draw_component<transform>("Transform", active_scene, &obj);
        draw_component<script_component>("Scripts", active_scene, &obj);
        draw_component<render_component>("Graphics Object", active_scene, &obj);
        draw_component<camera_component>("Camera", active_scene, &obj);
        draw_component<physics_component>("Physics Body", active_scene, &obj);
        draw_component<light_component>("Lights", active_scene, &obj);
        draw_component<animation_controller>("Animation Controller", active_scene, &obj);
      }

      ImGui::EndChild();
    }

  }  // namespace ui
}  // namespace other