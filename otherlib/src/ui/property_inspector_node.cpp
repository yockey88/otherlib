/**
 * \file ui/property_inspector_node.cpp
 **/
#include "ui/property_inspector_node.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/script_component.hpp"

#include "driver/driver.hpp"
#include "ui/colors.hpp"

IMGUI_REFLECT(glm::vec3, x, y, z);
IMGUI_REFLECT(glm::quat, w, x, y, z);

IMGUI_REFLECT(other::gpu::graphics_material, diffuse_color, diffuse_reflectivity, specular_color, specular_reflectivity, emissivity, transparency, shininess, padding);

IMGUI_REFLECT(other::scene_object, id, registry_id, name, visible);
IMGUI_REFLECT(other::transform, local_position, local_rotation_quat, local_scale);
IMGUI_REFLECT(other::script_component, script_object_id);
IMGUI_REFLECT(other::render_component, material, visible, animated);
IMGUI_REFLECT(other::physics_component, body, shape);

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
    }

    void property_inspector_node::on_render_node_body() {
      if (!ImGui::BeginChild("##property-inspector")) {
        ImGui::EndChild();
        return;
      }

      if (selected_object_ids.empty()) {
        scoped_color color_text(ImGuiCol_Text, colors::kTextFriendlyAlert);
        ImGui::Text("No object selected.");
      } else if (selected_object_ids.size() > 1) {
        scoped_color color_text(ImGuiCol_Text, colors::kTextBright);
        ImGui::Text("Multiple objects selected (%zu).", selected_object_ids.size());

      } else {
        natural_t obj_id = selected_object_ids.front();

        auto* active_scene = driver_ptr->get_active_scene();
        OTHER_ASSERT(active_scene != nullptr, "Active scene is null, cannot render properties.");

        scene_object& obj = active_scene->get_object(obj_id);

        scoped_color color_text(ImGuiCol_Text, colors::kTextBright);
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
        draw_component<script_component>("Script Component", active_scene, &obj);
        draw_component<render_component>("Render Component", active_scene, &obj);
        // draw_component<physics_component>("Physics Component", active_scene, &obj);
        draw_component<camera_component>("Camera Component", active_scene, &obj);
      }

      ImGui::EndChild();
    }

  }  // namespace ui
}  // namespace other