/**
 * \file ui/object-editor/property_inspector_node.cpp
 **/
#include "ui/object-editor/property_inspector_node.hpp"

#include <string>

#include <imgui/imgui.h>

#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "renderer/renderer_backend.hpp"

#include "object/animation_component.hpp"
#include "object/camera_component.hpp"
#include "object/grid_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "theme/colors.hpp"
#include "ui/component_widget.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/script/script_property_widget.hpp"

IMGUI_REFLECT(glm::vec3, x, y, z);
IMGUI_REFLECT(glm::quat, w, x, y, z);

IMGUI_REFLECT(other::scene_object, id, registry_id, name, visible);
IMGUI_REFLECT(other::transform, local_position, local_rotation_quat, local_scale);
IMGUI_REFLECT(other::script_component, script_object_id);
IMGUI_REFLECT(other::render_component, visible, tint);
IMGUI_REFLECT(other::model, name, submesh_indices);
IMGUI_REFLECT(other::physics_component, body, shape);
IMGUI_REFLECT(other::physics_body::settings, body_type, mass);
IMGUI_REFLECT(other::point_light_component, light);
IMGUI_REFLECT(other::direction_light_component, light);
IMGUI_REFLECT(other::grid_component, visible, show_axes, coordinate_system, plane, origin, cell_size, extent, major_line_every, sector_count, layer_extent, layer_spacing, line_width, line_color, major_line_color);
IMGUI_REFLECT(other::animation_component, clip_name, playing, looping, speed, time);

IMGUI_REFLECT(other::orthonormal_basis, i, j, k);
IMGUI_REFLECT(other::camera, position, direction, euler_angles, world_up, basis);
IMGUI_REFLECT(other::camera_component, camera);

namespace other {
  namespace ui {

    /// render components draw by hand: the material override is an asset *path* (raw text
    ///  field v1 — property_asset_slot drag-drop revival is phase 3), which the generic
    ///  reflected walk can't express for a natural_t id field
    template <>
    struct component_widget<render_component> {
      bool operator()(const std::string_view, render_component& comp, scene*, scene_object*, asset_handler* handler, driver* drvr) {
        OTHER_ASSERT(drvr != nullptr, "component_widget<render_component> needs a driver");
        bool changed = inspector::property_bool("Visible", comp.visible);
        changed |= inspector::property_vec4("Tint", comp.tint, 0.01f);

        std::string current_path;
        if (comp.material_asset_id != 0 && handler != nullptr) {
          if (const asset* mat_asset = handler->get_asset(comp.material_asset_id); mat_asset != nullptr) {
            current_path = mat_asset->load_path.generic_string();
          }
        }
        char path_buf[512];
        std::strncpy(path_buf, current_path.c_str(), sizeof(path_buf));
        path_buf[sizeof(path_buf) - 1] = '\0';
        if (inspector::property_text("Material", path_buf, sizeof(path_buf))) {
          const std::string new_path = path_buf;
          if (new_path.empty()) {
            comp.material_asset_id = 0;
            comp.last_material_asset_id = 0;
            changed = true;
          } else if (const filepath p{ new_path }; std::filesystem::is_regular_file(p) && p.extension() == ".omat") {
            /// commit only when the text points at a real material — partial paths while
            ///  typing stay inert
            const natural_t material_id = drvr->begin_asset_load(p);
            if (material_id != 0 && material_id != comp.material_asset_id) {
              comp.material_asset_id = material_id;
              comp.last_material_asset_id = material_id;
              changed = true;
            }
          }
        }
        return changed;
      }
    };

    property_inspector_node::property_inspector_node(editor_context& ctx, ui_window* window, driver* drvr)
        : ui_node(window, "Property Inspector"), context(ctx), driver_ptr(drvr) {
    }

    template <typename T>
    void property_inspector_node::draw_component_section(const std::string_view component_name, const glm::vec4& color, scene* active_scene, scene_object* object, on_component_modified_fn<T> on_modified) {
      OTHER_ASSERT(active_scene != nullptr, "Active scene must not be nullptr in draw_component_section");
      OTHER_ASSERT(object != nullptr, "Object must not be nullptr in draw_component_section");
      if (!active_scene->has_component<T>(object)) {
        return;
      }

      T* comp = active_scene->get_component<T>(object);
      OTHER_ASSERT(comp != nullptr, "Component of type '{}' not found on object with ID {}", component_name, object->id);

      inspector::component_section_flags section_flags{};
      if constexpr (std::is_same_v<T, transform>) {
        section_flags.removable = false;
      }

      bool remove_requested = false;
      bool is_open = inspector::begin_component_section(component_name, color, section_flags, &remove_requested);

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
        context.notify_scene_edited();
        if (on_modified != nullptr) {
          on_modified(comp, object, active_scene, driver_ptr);
        }
      }

      /// \todo flesh this out more, this could be it but it may be more complicated
      if (remove_requested) {
        // active_scene->remove_component<T>(object);
      }
    }

    template <typename T>
    bool property_inspector_node::draw_component_selector(const std::string_view component_name, scene* active_scene, scene_object* object) {
      OTHER_ASSERT(active_scene != nullptr, "Active scene must not be nullptr in draw_add_component_button");
      OTHER_ASSERT(object != nullptr, "Object must not be nullptr in draw_add_component_button");
      if (active_scene->has_component<T>(object)) {
        return false;
      }
    }

    void property_inspector_node::on_render_node_body() {
      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();

      if (!context.has_selection()) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No object selected.");
      } else if (active_scene == nullptr) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No active scene.");
      } else if (context.multi_select_enabled()) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextBright));
        ImGui::Text("Multiple objects selected (%zu).", context.current_selection.objects.size());
      } else {
        OTHER_ASSERT(active_scene != nullptr, "Active scene must not be nullptr");
        natural_t obj_id = context.current_selection.objects.front();
        /// selection ids die when a snapshot restore (play/stop, undo) reassigns runtime
        ///  ids — a stale entry is dropped, never dereferenced
        scene_object* selected = active_scene->find_object(obj_id);
        if (selected == nullptr) {
          context.current_selection.objects.clear();
          scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
          ImGui::Text("No object selected.");
          return;
        }
        scene_object& obj = *selected;

        char name_buf[256];
        std::strncpy(name_buf, obj.name.c_str(), sizeof(name_buf));
        name_buf[sizeof(name_buf) - 1] = '\0';

        if (inspector::draw_object_header_editable(name_buf, sizeof(name_buf), obj.id, colors::scene_object::kSignature)) {
          obj.name = std::string(name_buf);
          context.notify_scene_edited();
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
        draw_component_section<transform>("Transform", colors::scene_object::kComponentTransform, active_scene, &obj);
        draw_component_section<script_component>("Scripts", colors::scene_object::kComponentScript, active_scene, &obj);
        draw_component_section<render_component>(
          "Graphics Object", colors::scene_object::kComponentRenderer, active_scene, &obj,
          [](render_component* comp, scene_object* object, scene* active_scene, driver* drvr) {
            OTHER_ASSERT(comp != nullptr, "Render component is null in on_modified callback");
            OTHER_ASSERT(object != nullptr, "Scene object is null in render_component on_modified callback");
            OTHER_ASSERT(active_scene != nullptr, "Active scene is null in render_component on_modified callback");
            OTHER_ASSERT(drvr != nullptr, "Driver is null in render_component on_modified callback");
            natural_t new_asset_id = comp->model_asset_id;
            if (new_asset_id == 0) {
              /// tint/material edits fire this too; nothing to re-validate without a model
              return;
            }

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

            } else {
              CORE_LOG_ERROR("Error: Asset ID {} is not a valid model source asset in render_component on_modified callback", new_asset_id);
            }
          });
        draw_component_section<physics_component>(
          "Physics Object", colors::scene_object::kComponentPhysics, active_scene, &obj,
          [](physics_component* comp, scene_object* object, scene* active_scene, driver* drvr) {

          });
        draw_component_section<camera_component>("Camera", colors::scene_object::kComponentCamera, active_scene, &obj);
        draw_component_section<grid_component>("Grid", colors::scene_object::kComponentGrid, active_scene, &obj);
        draw_component_section<physics_component>("Physics Body", colors::scene_object::kComponentPhysics, active_scene, &obj);
        draw_component_section<point_light_component>("Point Light", colors::scene_object::kComponentPointLight, active_scene, &obj);
        draw_component_section<direction_light_component>("Direction Light", colors::scene_object::kComponentDirectionLight, active_scene, &obj);
        draw_component_section<animation_component>("Animation", colors::scene_object::kComponentAnimation, active_scene, &obj);

        const std::string button_str = std::format("Add Component##{}", obj.name);
        if (inspector::draw_add_component_button(button_str)) {
          /// \todo open component picker popup
          ImGui::OpenPopup("##add_component_popup");
        }

        /// \todo component picker popup
        constexpr ImVec2 picker_size = { 800.f, 600.f };
        ImGui::SetNextWindowSize(picker_size);
        if (ImGui::BeginPopup("##add_component_popup")) {
          ImGui::BeginChild("##add_component_child");

          auto* draw_list = ImGui::GetWindowDrawList();

          ImVec2 picker_pos = ImGui::GetWindowPos();
          ImVec2 picker_size = ImGui::GetWindowSize();
          ImVec2 picker_end = { picker_pos.x + picker_size.x, picker_pos.y + picker_size.y };
          draw_list->AddRectFilled(picker_pos, picker_end, colors::to_im_col({ 0.2f, 0.2f, 0.2f, 1.f }));

          auto& kernel = driver_ptr->get_kernel();
          auto& scene_sys = kernel.get_core_system<scene_system>();
          auto& comp_registry = scene_sys.get_component_registry();

          for (const auto& [comp_name, comp_info] : comp_registry.get_registry()) {
            if (comp_info.has_component(active_scene, &obj)) {
              continue;
            }

            if (ImGui::Selectable(comp_info.component_name.c_str())) {
              OTHER_ASSERT(comp_info.add_component != nullptr, "Add component function is null for component '{}'", comp_info.component_name);
              comp_info.add_component(active_scene, &obj);
              context.notify_scene_edited();
              ImGui::CloseCurrentPopup();
            }
          }

          ImGui::EndChild();
          ImGui::EndPopup();
        }
      }
    }

  }  // namespace ui
}  // namespace other