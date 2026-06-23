/**
 * \file ui/object-editor/property_inspector_node.cpp
 **/
#include "ui/object-editor/property_inspector_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "renderer/renderer_backend.hpp"
#include "renderer/ui/colors.hpp"

#include "object/animation_controller.hpp"
#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "ui/component_widget.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/script/script_property_widget.hpp"

IMGUI_REFLECT(glm::vec3, x, y, z);
IMGUI_REFLECT(glm::quat, w, x, y, z);

IMGUI_REFLECT(other::gpu::graphics_material, diffuse_color, diffuse_reflectivity, specular_color, specular_reflectivity, emissivity, transparency, shininess);

IMGUI_REFLECT(other::scene_object, id, registry_id, name, visible);
IMGUI_REFLECT(other::transform, local_position, local_rotation_quat, local_scale);
IMGUI_REFLECT(other::script_component, script_object_id);
IMGUI_REFLECT(other::render_component, material, visible, animated);
IMGUI_REFLECT(other::model, name, submesh_indices);
IMGUI_REFLECT(other::physics_component, body, shape);
IMGUI_REFLECT(other::physics_body_settings, body_type, mass);
IMGUI_REFLECT(other::point_light, position, color);
IMGUI_REFLECT(other::direction_light, direction, color);

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
    void property_inspector_node::draw_component_section(const std::string_view component_name, scene* active_scene, scene_object* object, on_component_modified_fn<T> on_modified) {
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

      bool changed = false;
      OTHER_ASSERT(driver_ptr != nullptr, "Driver is null");
      auto& assets = driver_ptr->get_kernel().get_core_system<asset_system>();
      asset_handler* handler = assets.get_asset_manager().get();
      OTHER_ASSERT(handler != nullptr, "Asset handler is null");
      if (is_open) {
        changed = component_widget<T>{}(component_name, *comp, active_scene, object, handler, driver_ptr);
      }

      inspector::end_component_section();

      if (changed) {
        if (on_modified != nullptr) {
          on_modified(comp, object, active_scene, driver_ptr);
        }
      }

      /// \todo flesh this out more, this could be it but it may be more complicated
      if (remove_requested) {
        // active_scene->remove_component<T>(object);
      }
    }

    void property_inspector_node::on_render_node_body() {
      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();
      if (active_scene == nullptr) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No active scene.");
      } else if (selected_object_ids.empty()) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No object selected.");
      } else if (selected_object_ids.size() > 1) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextBright));
        ImGui::Text("Multiple objects selected (%zu).", selected_object_ids.size());
      } else {
        OTHER_ASSERT(active_scene != nullptr, "Active scene must not be nullptr");
        natural_t obj_id = selected_object_ids.front();
        scene_object& obj = active_scene->get_object(obj_id);

        char name_buf[256];
        std::strncpy(name_buf, obj.name.c_str(), sizeof(name_buf));
        name_buf[sizeof(name_buf) - 1] = '\0';

        if (inspector::draw_object_header_editable(name_buf, sizeof(name_buf), obj.id, colors::scene_object::kSignature)) {
          obj.name = std::string(name_buf);
        }
        ImGui::Separator();

        // render list of children names
        auto children_ids = active_scene->get_children_ids(obj.id);
        if (!children_ids.empty()) {
          std::stringstream ss;
          for (natural_t child_id : children_ids) {
            scene_object* child = active_scene->find_object(child_id);
            if (child != nullptr) {
              ss << child->name;
              if (child_id != children_ids.back()) {
                ss << ", ";
              }
            }
          }

          std::string children_names = ss.str();
          ImGui::Text("Children: [%s]", children_names.c_str());
        }

        /// components
        draw_component_section<transform>("Transform", active_scene, &obj);
        draw_component_section<script_component>("Scripts", active_scene, &obj);
        draw_component_section<render_component>(
          "Graphics Object", active_scene, &obj,
          [](render_component* comp, scene_object* object, scene* active_scene, driver* drvr) {
            natural_t new_asset_id = comp->model_asset_id;

            auto& assets = drvr->get_kernel().get_core_system<asset_system>();
            auto& handler = assets.get_asset_manager();
            OTHER_ASSERT(handler != nullptr, "Asset handler is null in render_component on_modified callback");

            auto* asset = handler->get_loaded_asset(new_asset_id);
            OTHER_ASSERT(asset != nullptr, "Model asset ID {} not found in render_component on_modified callback", new_asset_id);
            if (asset->asset_type == asset::type::MODEL_SOURCE) {
              auto* renderer = subsystem<renderer_backend>::get();
              OTHER_ASSERT(renderer != nullptr, "Renderer backend is null in render_component on_modified callback");
              ref<model_source> model_src = renderer->get_model_source(asset->path_hash);
              OTHER_ASSERT(model_src != nullptr, "Model source not found for asset ID {} in render_component on_modified callback", new_asset_id);

              // comp->obj_model = model_src->produce_model(const std::string &name)

            } else if (asset->asset_type == asset::type::MODEL) {
            } else {
              CORE_LOG_ERROR("Error: Asset ID {} is not a valid model or model source asset in render_component on_modified callback", new_asset_id);
            }
          });
        draw_component_section<camera_component>("Camera", active_scene, &obj);
        draw_component_section<physics_component>("Physics Body", active_scene, &obj);
        draw_component_section<point_light_component>("Lights", active_scene, &obj);
        draw_component_section<animation_controller>("Animation Controller", active_scene, &obj);

        if (inspector::draw_add_component_button()) {
          /// \todo open component picker popup
          ImGui::OpenPopup("##add_component_popup");
        }

        /// \todo component picker popup
        if (ImGui::BeginPopup("##add_component_popup")) {
          {
            scoped_color text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kText));
            ImGui::Text("Add Component...");
            ImGui::Separator();
          }
          /// list available component types here
          ImGui::EndPopup();
        }
      }
    }

  }  // namespace ui
}  // namespace other