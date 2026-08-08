/**
 * \file ui/node-editor/node_editor_display.cpp
 **/
#include "ui/node-editor/node_editor_display.hpp"

#include <set>
#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "theme/colors.hpp"
#include "ui/node-editor/node_editor_canvas.hpp"
#include "ui/type-bindings/value_ui.hpp"
#include "ui/ui_helpers.hpp"

namespace other {
  namespace ui {

    // node_editor_display::node_editor_display() {
    //   // canvas_id = add_child_node(make_ref<node_editor_canvas_node>(this));
    //   // events.add_listener("remove-node", [this](const value& data) {
    //   //   natural_t node_id = data;
    //   //   auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
    //   //   if (itr != node_data.end()) {
    //   //     node_data.erase(itr);

    //   //     auto& canvas = get_node_as<node_editor_canvas_node>(canvas_id);
    //   //     canvas.remove_single_node(node_id);
    //   //   }
    //   // });

    //   // events.add_listener("break-node-link", [this](const value& data) {
    //   //   struct link_info {
    //   //     natural_t node_id;
    //   //     natural_t link_id;
    //   //   };
    //   //   link_info info = data;
    //   //   auto itr = std::find_if(node_data.begin(), node_data.end(), [info](const auto& pair) { return pair.second.node_id == info.node_id; });
    //   //   if (itr != node_data.end()) {
    //   //     if (itr->second.remove_link_fn) {
    //   //       itr->second.remove_link_fn(info.node_id, info.link_id);
    //   //     }
    //   //   }
    //   // });
    // }

    natural_t node_editor_display::add_editor_node(const std::string_view node_name, uint8_t input_pins, uint8_t output_pins) {
      natural_t name_hash = FNV(node_name);
      natural_t node_id = canvas.create_single_node(node_name, input_pins, output_pins);

      auto [itr, inserted] = node_data.emplace(name_hash, node_display_data{ node_id, name_hash });
      OTHER_ASSERT(inserted, "Node with name '{}' already exists in node editor", node_name);
      return node_id;
    }

    void node_editor_display::connect_node_pins(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx) {
      PROFILE_SECTION("node_editor_display::connect_node_pins");
      natural_t from_name_hash = FNV(from_node);
      natural_t to_name_hash = FNV(to_node);

      auto from_itr = node_data.find(from_name_hash);
      if (from_itr == node_data.end()) {
        CORE_LOG_ERROR("From node '{}' not found in node editor!", from_node);
        return;
      }

      auto to_itr = node_data.find(to_name_hash);
      if (to_itr == node_data.end()) {
        CORE_LOG_ERROR("To node '{}' not found in node editor!", to_node);
        return;
      }

      canvas.connect_node_pins(from_itr->second.node_id, from_pin_idx, to_itr->second.node_id, to_pin_idx);
      if (from_itr->second.add_link_fn) {
        from_itr->second.add_link_fn(from_itr->second.node_id, from_pin_idx, to_pin_idx);
      }
      if (to_itr->second.add_link_fn) {
        to_itr->second.add_link_fn(to_itr->second.node_id, from_pin_idx, to_pin_idx);
      }
    }

    void node_editor_display::clear_nodes() {
      canvas.clear_all_nodes();
      node_data.clear();
    }

    void node_editor_display::reorganize_nodes() {
      PROFILE_SECTION("node_editor_display::reorganize_nodes");
      /// layout: build the adjacency graph, topological sort it, then place each node by column
      ///  (sort order) and stagger rows so overlapping outputs spread out

      /// graph data
      ostd::map<natural_t, ostd::vector<natural_t>> adjacency_list;
      for (natural_t i = 0; i < canvas.links.link_start_pin_indices.size(); ++i) {
        natural_t start_pin = canvas.links.link_start_pin_indices[i];
        natural_t end_pin = canvas.links.link_end_pin_indices[i];

        natural_t from_node = static_cast<natural_t>(-1);
        natural_t to_node = static_cast<natural_t>(-1);

        for (natural_t m = 0; m < canvas.nodes.node_output_pin_indices.size(); ++m) {
          auto& output_pins = canvas.nodes.node_output_pin_indices[m];
          if (std::ranges::find(output_pins, start_pin) != output_pins.end()) {
            from_node = m;
            break;
          }
        }
        for (natural_t m = 0; m < canvas.nodes.node_input_pin_indices.size(); ++m) {
          auto& input_pins = canvas.nodes.node_input_pin_indices[m];
          if (std::ranges::find(input_pins, end_pin) != input_pins.end()) {
            to_node = m;
            break;
          }
        }

        if (from_node != static_cast<natural_t>(-1) && to_node != static_cast<natural_t>(-1)) {
          adjacency_list[from_node].emplace_back(to_node);
        }
      }

      /// topological sort
      ostd::vector<natural_t> in_degree;
      ostd::vector<natural_t> sorted;
      in_degree.resize(canvas.nodes.node_names.size(), 0);
      sorted.reserve(in_degree.size());

      std::set<natural_t> no_incoming_edges = {};
      for (natural_t n = 0; n < canvas.nodes.node_names.size(); ++n) {
        for (const auto& neighbor : adjacency_list[n]) {
          in_degree[neighbor]++;
        }
        if (in_degree[n] == 0) {
          no_incoming_edges.insert(n);
        }
      }

      /// build list of edges to process
      ostd::map<natural_t, std::set<natural_t>> edges;
      for (const auto& [nid, neighbors] : adjacency_list) {
        for (const auto& neighbor : neighbors) {
          edges[nid].insert(neighbor);
        }
      }

      while (!no_incoming_edges.empty()) {
        natural_t current = *no_incoming_edges.begin();
        no_incoming_edges.erase(no_incoming_edges.begin());
        sorted.push_back(current);

        auto& current_edges = edges[current];
        while (!current_edges.empty()) {
          natural_t neighbor = *current_edges.begin();
          current_edges.erase(current_edges.begin());

          in_degree[neighbor]--;
          if (in_degree[neighbor] == 0) {
            no_incoming_edges.insert(neighbor);
          }
        }
      }

      bool any_cycles = std::ranges::any_of(in_degree, [](natural_t degree) { return degree > 0; });
      if (any_cycles) {
        CORE_LOG_WARN("Node editor graph has cycles, cannot reorganize nodes!");
        return;
      }

      /// first place all nodes with no inputs in the first col
      /// then place all nodes connected to those nodes in the next col, etc
      ostd::map<natural_t, natural_t> node_columns;
      for (const auto& node_id : sorted) {
        natural_t col = 0;
        for (const auto& [from_node, neighbors] : adjacency_list) {
          if (std::ranges::find(neighbors, node_id) != neighbors.end()) {
            natural_t neighbor_col = node_columns[from_node] + 1;
            if (neighbor_col > col) {
              col = neighbor_col;
            }
          }
        }
        node_columns[node_id] = col;
      }

      ostd::map<natural_t, natural_t> column_node_counts;
      for (const auto& node_id : sorted) {
        natural_t col = node_columns[node_id];
        natural_t row = column_node_counts[col]++;
        canvas.nodes.node_positions[node_id] = glm::vec2{
          static_cast<float>(col) * (node_editor_canvas::kMinNodeWidth + 15.f),
          static_cast<float>(row) * (node_editor_canvas::kMinNodeHeight + 25.f)
        };
      }
    }

    void node_editor_display::on_prepare_render() {
      ImGui::PushStyleColor(ImGuiCol_ChildBg, colors::rgba_to_imvec4(colors::kNodeEditorBackground));
    }

    void node_editor_display::on_render_end() {
      ImGui::PopStyleColor();
    }

    void node_editor_display::render_node_body(natural_t node_id, ImRect body_rect) {
      auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
      if (itr != node_data.end()) {
        if (itr->second.display_fn) {
          itr->second.display_fn(node_id, body_rect);
        }
      }
    }

    void node_editor_display::set_display_fn(natural_t node_id, node_display_fn_t display_fn) {
      auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
      if (itr != node_data.end()) {
        itr->second.display_fn = display_fn;
      }
    }

    void node_editor_display::set_add_link_fn(natural_t node_id, node_add_link_fn_t add_link_fn) {
      auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
      if (itr != node_data.end()) {
        itr->second.add_link_fn = add_link_fn;
      }
    }

    void node_editor_display::set_remove_link_fn(natural_t node_id, node_remove_link_fn_t remove_link_fn) {
      auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
      if (itr != node_data.end()) {
        itr->second.remove_link_fn = remove_link_fn;
      }
    }

    void node_editor_display::set_update_fn(natural_t node_id, node_update_fn_t update_fn) {
      auto itr = std::find_if(node_data.begin(), node_data.end(), [node_id](const auto& pair) { return pair.second.node_id == node_id; });
      if (itr != node_data.end()) {
        itr->second.update_fn = update_fn;
      }
    }

  }  // namespace ui
}  // namespace other