/**
 * \file ui/object-editor/property_inspector_node.cpp
 **/
#include "ui/object-editor/property_inspector_node.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include <imgui/imgui.h>

#include "core/profiler.hpp"
#include "math/orthonormal_basis.hpp"
#include "serialization/reflection.hpp"

#include "object/animation_component.hpp"
#include "object/audio_listener_component.hpp"
#include "object/audio_source_component.hpp"
#include "object/camera_component.hpp"
#include "object/grid_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/physics_joint_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/systems/asset_system.hpp"
#include "driver/systems/scene_system.hpp"
#include "theme/colors.hpp"
#include "ui/asset_picker.hpp"
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
IMGUI_REFLECT(other::physics_component, settings);
IMGUI_REFLECT(other::physics_body::settings, body_type, mass, friction, restitution, linear_damping, angular_damping, gravity_factor, is_trigger, continuous_cd, shape);
IMGUI_REFLECT(other::physics_shape_desc, shape_kind, half_extents, radius, half_height, fit_render_bounds);
IMGUI_REFLECT(other::physics_joint_component, target_object_name, break_force, broken);
IMGUI_REFLECT(other::point_light_component, light);
IMGUI_REFLECT(other::direction_light_component, light);
IMGUI_REFLECT(other::grid_component, visible, show_axes, coordinate_system, plane, origin, cell_size, extent, major_line_every, sector_count, layer_extent, layer_spacing, line_width, line_color, major_line_color);
IMGUI_REFLECT(other::animation_component, clip_name, playing, looping, speed, time);
IMGUI_REFLECT(other::audio_source_component, playing, looping, volume, pitch, bus, spatial, min_distance, max_distance, doppler_factor);
IMGUI_REFLECT(other::audio_listener_component, active);

IMGUI_REFLECT(other::orthonormal_basis, i, j, k);
IMGUI_REFLECT(other::camera, position, direction, euler_angles, world_up, basis);
IMGUI_REFLECT(other::camera_component, camera);

namespace other {
  namespace ui {

    /// render components draw by hand: model/material go through the asset slot + picker,
    ///  and the reflected walk would also surface obj_model/last_* internals
    template <>
    struct component_widget<render_component> {
      bool operator()(const std::string_view, render_component& comp, scene*, scene_object*, asset_handler* handler, driver* drvr) {
        OTHER_ASSERT(drvr != nullptr, "component_widget<render_component> needs a driver");
        bool changed = inspector::property_bool("Visible", comp.visible);
        changed |= inspector::property_vec4("Tint", comp.tint, 0.01f);
        /// prepare_render_data validates the new ids and rebuilds obj_model once loaded
        changed |= inspector::property_asset_field("Model", comp.model_asset_id, asset::MODEL_SOURCE, handler, drvr);
        changed |= inspector::property_asset_field("Material", comp.material_asset_id, asset::MATERIAL, handler, drvr);
        return changed;
      }
    };

    /// audio sources draw by hand: the clip goes through the asset slot and the bus wants a combo
    template <>
    struct component_widget<audio_source_component> {
      bool operator()(const std::string_view, audio_source_component& comp, scene*, scene_object*, asset_handler* handler, driver* drvr) {
        OTHER_ASSERT(drvr != nullptr, "component_widget<audio_source_component> needs a driver");
        bool changed = inspector::property_asset_field("Clip", comp.clip_asset_id, asset::AUDIO, handler, drvr);

        changed |= inspector::property_bool("Playing", comp.playing);
        changed |= inspector::property_bool("Looping", comp.looping);
        changed |= inspector::property_float("Volume", comp.volume, 0.01f);
        changed |= inspector::property_float("Pitch", comp.pitch, 0.01f);

        constexpr std::array<const char*, 4> kBusNames = { "Master", "Music", "SFX", "UI" };
        int bus_index = static_cast<int>(std::min<uint32_t>(comp.bus, static_cast<uint32_t>(kBusNames.size()) - 1));
        if (ImGui::Combo("Bus", &bus_index, kBusNames.data(), static_cast<int>(kBusNames.size()))) {
          comp.bus = static_cast<uint32_t>(bus_index);
          changed = true;
        }

        changed |= inspector::property_bool("Spatial", comp.spatial);
        if (comp.spatial) {
          changed |= inspector::property_float("Min Distance", comp.min_distance, 0.1f);
          changed |= inspector::property_float("Max Distance", comp.max_distance, 1.f);
          changed |= inspector::property_float("Doppler", comp.doppler_factor, 0.01f);
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
      PROFILE_SECTION("property_inspector_node::draw_component_section");
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

      if (remove_requested) {
        active_scene->remove_component<T>(object);
        context.notify_scene_edited();
      }
    }

    void property_inspector_node::on_render_node_body() {
      PROFILE_SECTION("property_inspector_node::on_render_node_body");
      auto& scenes = driver_ptr->get_kernel().get_core_system<scene_system>();
      auto* active_scene = scenes.get_active_scene();

      if (!context.has_selection()) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No object selected.");
      } else if (active_scene == nullptr) {
        scoped_color color_text(ImGuiCol_Text, colors::rgba_to_imvec4(colors::kTextFriendlyAlert));
        ImGui::Text("No active scene.");
      } else if (context.current_selection.objects.size() > 1) {
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
        /// model/material id edits need no callback — prepare_render_data validates and
        ///  rebuilds obj_model once the async load lands
        draw_component_section<render_component>("Graphics Object", colors::scene_object::kComponentRenderer, active_scene, &obj);
        draw_component_section<physics_component>("Physics Object", colors::scene_object::kComponentPhysics, active_scene, &obj);
        draw_component_section<physics_joint_component>("Physics Joint", colors::scene_object::kComponentPhysics, active_scene, &obj);
        draw_component_section<camera_component>("Camera", colors::scene_object::kComponentCamera, active_scene, &obj);
        draw_component_section<grid_component>("Grid", colors::scene_object::kComponentGrid, active_scene, &obj);
        draw_component_section<point_light_component>("Point Light", colors::scene_object::kComponentPointLight, active_scene, &obj);
        draw_component_section<direction_light_component>("Direction Light", colors::scene_object::kComponentDirectionLight, active_scene, &obj);
        draw_component_section<animation_component>("Animation", colors::scene_object::kComponentAnimation, active_scene, &obj);
        draw_component_section<audio_source_component>("Audio Source", colors::scene_object::kComponentAudio, active_scene, &obj);
        draw_component_section<audio_listener_component>("Audio Listener", colors::scene_object::kComponentAudio, active_scene, &obj);

        const std::string button_str = std::format("Add Component##{}", obj.name);
        if (inspector::draw_add_component_button(button_str)) {
          ImGui::OpenPopup("##add_component_popup");
        }

        ImGui::SetNextWindowSize({ 300.f, 0.f });
        if (ImGui::BeginPopup("##add_component_popup")) {
          PROFILE_SECTION("property_inspector_node::on_render_node_body--add_component_popup");
          static char comp_search[64] = {};
          if (ImGui::IsWindowAppearing()) {
            comp_search[0] = '\0';
            ImGui::SetKeyboardFocusHere();
          }
          ImGui::SetNextItemWidth(-FLT_MIN);
          ImGui::InputTextWithHint("##comp_search", "search components...", comp_search, sizeof(comp_search));
          ImGui::Separator();

          std::string needle = comp_search;
          std::ranges::transform(needle, needle.begin(), [](unsigned char c) { return std::tolower(c); });

          auto& comp_registry = driver_ptr->get_kernel().get_core_system<scene_system>().get_component_registry();
          uint32_t shown = 0;
          if (ImGui::BeginChild("##add_component_list", { 0.f, 220.f })) {
            for (const auto& [comp_name, comp_info] : comp_registry.get_registry()) {
              if (comp_info.has_component(active_scene, &obj)) {
                continue;
              }

              if (!needle.empty()) {
                std::string name_lower = comp_info.component_name;
                std::ranges::transform(name_lower, name_lower.begin(), [](unsigned char c) { return std::tolower(c); });
                if (name_lower.find(needle) == std::string::npos) {
                  continue;
                }
              }
              ++shown;

              if (ImGui::Selectable(comp_info.component_name.c_str())) {
                OTHER_ASSERT(comp_info.add_component != nullptr, "Add component function is null for component '{}'", comp_info.component_name);
                comp_info.add_component(active_scene, &obj);
                context.notify_scene_edited();
                ImGui::CloseCurrentPopup();
              }
            }

            if (shown == 0) {
              ImGui::TextDisabled("no matching components");
            }
          }
          ImGui::EndChild();
          ImGui::EndPopup();
        }
      }
    }

  }  // namespace ui
}  // namespace other