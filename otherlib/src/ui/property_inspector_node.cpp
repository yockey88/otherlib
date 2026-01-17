/**
 * \file ui/property_inspector_node.cpp
 **/
#include "ui/property_inspector_node.hpp"

#include "object/component_registry.hpp"

#include "driver/driver.hpp"
#include "ui/colors.hpp"

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
        // component_registry* comp_reg = active_scene->get_component<component_registry>(&obj);
        // if (comp_reg != nullptr) {
        //   ImGui::Text("Components:");
        // }
      }

      ImGui::EndChild();
    }

  }  // namespace ui
}  // namespace other