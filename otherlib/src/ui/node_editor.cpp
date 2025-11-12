/**
 * \file ui/node_editor.cpp
 **/
#include "ui/node_editor.hpp"

#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/logger.hpp"

#include "renderer/ui/ui_helpers.hpp"

#include "ui/colors.hpp"
#include "ui/node_editor_canvas_node.hpp"

#include "colors.hpp"

namespace other {
  namespace ui {

    node_editor::node_editor(event_system& events)
        : ui_window(events, "Node Editor", true, ImGuiWindowFlags_NoCollapse) {
      canvas_id = add_node(make_scope<node_editor_canvas_node>(this));
    }

    void node_editor::add_editor_node(const std::string_view node_name, uint8_t input_pins, uint8_t output_pins) {
      natural_t name_hash = FNV(node_name);

      auto& canvas = get_node_as<node_editor_canvas_node>(canvas_id);
      natural_t node_id = canvas.create_single_node(node_name, input_pins, output_pins);

      auto [itr, inserted] = node_name_hashes.emplace(name_hash, node_id);
      OTHER_ASSERT(inserted, "Node with name '{}' already exists in node editor", node_name);
    }

    void node_editor::connect_node_pins(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx) {
      natural_t from_name_hash = FNV(from_node);
      natural_t to_name_hash = FNV(to_node);

      auto from_itr = node_name_hashes.find(from_name_hash);
      if (from_itr == node_name_hashes.end()) {
        CORE_LOG_ERROR("From node '{}' not found in node editor!", from_node);
        return;
      }

      auto to_itr = node_name_hashes.find(to_name_hash);
      if (to_itr == node_name_hashes.end()) {
        CORE_LOG_ERROR("To node '{}' not found in node editor!", to_node);
        return;
      }

      auto& canvas = get_node_as<node_editor_canvas_node>(canvas_id);
      canvas.connect_node_pins(from_itr->second, from_pin_idx, to_itr->second, to_pin_idx);
    }

    void node_editor::on_pre_render_nodes() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::editor::kNodeEditorBackground);
    }

    void node_editor::on_post_render_nodes() {
      ImGui::PopStyleColor();
    }

  }  // namespace ui
}  // namespace other