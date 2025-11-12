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

    void node_editor::on_pre_render_nodes() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::editor::kNodeEditorBackground);
    }

    void node_editor::on_post_render_nodes() {
      ImGui::PopStyleColor();
    }

  }  // namespace ui
}  // namespace other