/**
 * \file ui/node_editor_canvas_node.cpp
 **/
#include "ui/node_editor_canvas_node.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "renderer/ui/ui_helpers.hpp"

#include "ui/colors.hpp"
#include "ui/node_editor.hpp"
#include "ui/node_editor_canvas_node.hpp"

namespace other {
  namespace ui {

    natural_t node_editor_canvas_node::node_data::create(const std::string_view node_name, const glm::vec2& position, const glm::vec2& size, const glm::vec4& header_color, const glm::vec4& body_color) {
      natural_t node_id = node_names.size();

      node_names.emplace_back(std::string{ node_name });
      node_positions.emplace_back(position);
      node_sizes.emplace_back(size);
      node_header_colors.emplace_back(header_color);
      node_body_colors.emplace_back(body_color);
      node_input_pin_indices.emplace_back();
      node_output_pin_indices.emplace_back();
      interactables.emplace_back();
      node_states.emplace_back(node_state::NORMAL);

      global_node_positions.emplace_back(glm::vec2(0.f, 0.f));
      full_node_maxs.emplace_back(glm::vec2(0.f, 0.f));
      node_header_ends.emplace_back(glm::vec2(0.f, 0.f));
      node_body_begins.emplace_back(glm::vec2(0.f, 0.f));

      return node_id;
    }

    natural_t node_editor_canvas_node::pin_data::create(natural_t node_id, natural_t pin_index, pin_type type) {
      natural_t pin_id = pin_node_indices.size();

      pin_node_indices.emplace_back(node_id);
      pin_indices_within_node.emplace_back(pin_index);
      pin_positions.emplace_back(glm::vec2(0.f, 0.f));
      pin_radii.emplace_back(4.75f);
      pin_node_relative_positions.emplace_back(glm::vec2(0.f, 0.f));
      pin_colors.emplace_back(glm::vec4(1.f, 1.f, 1.f, 1.f));
      interactables.emplace_back();
      pin_states.emplace_back(pin_state::NORMAL);
      pin_types.emplace_back(type);

      return pin_id;
    }

    natural_t node_editor_canvas_node::link_data::create(natural_t start_pin_idx, natural_t end_pin_idx, const glm::vec4& color) {
      natural_t link_id = link_start_pin_indices.size();

      link_start_pin_indices.emplace_back(start_pin_idx);
      link_end_pin_indices.emplace_back(end_pin_idx);
      for (size_t i = 0; i < kNumBezierPoints; ++i) {
        link_bezier_points[i].emplace_back(glm::vec2(0.f));
      }
      link_colors.emplace_back(color);

      return link_id;
    }

    node_editor_canvas_node::node_editor_canvas_node(node_editor* parent)
        : ui_node((ui_window*)parent, "Node Editor Canvas", glm::vec2(0, 0), ImGuiChildFlags_Borders /* | ImGuiChildFlags_FrameStyle */, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar), editor(parent) {
    }

    natural_t node_editor_canvas_node::create_single_node(const std::string_view node_name, uint8_t input_pins, uint8_t output_pins) {
      glm::vec2 position = glm::vec2(0.f, 0.f);
      glm::vec2 size = glm::vec2(kMinNodeWidth, kMinNodeHeight);

      // calculate a position and size so it isn't overlapping other nodes
      float shift = 0.f;
      for (size_t i = 0; i < nodes.node_positions.size(); ++i) {
        position.y += size.y + 33.f + shift;
        if (position.x > canvas_size.x) {
          position.x = 0.f;
          position.y += size.y + 50.f + shift;

          if (position.x > canvas_size.x) {
            position.x = 0.f;
            shift += 7.5f;
          }
        }
      }

      glm::vec4 header_color = { colors::kNodeHeaderColor.x, colors::kNodeHeaderColor.y, colors::kNodeHeaderColor.z, colors::kNodeHeaderColor.w };
      glm::vec4 body_color = { colors::kNodeBodyColor.x, colors::kNodeBodyColor.y, colors::kNodeBodyColor.z, colors::kNodeBodyColor.w };
      natural_t node_id = nodes.create(node_name, position, size, header_color, body_color);

      nodes.node_input_pin_indices[node_id].reserve(input_pins);
      nodes.node_output_pin_indices[node_id].reserve(output_pins);

      for (uint8_t i = 0; i < input_pins; ++i) {
        natural_t pin_id = pins.create(node_id, i, pin_data::pin_type::INPUT);
        nodes.node_input_pin_indices[node_id].emplace_back(pin_id);
      }

      for (uint8_t i = 0; i < output_pins; ++i) {
        natural_t pin_id = pins.create(node_id, i, pin_data::pin_type::OUTPUT);
        nodes.node_output_pin_indices[node_id].emplace_back(pin_id);
      }

      return node_id;
    }

    void node_editor_canvas_node::remove_single_node(natural_t node_id) {
      auto node_pins = { nodes.node_input_pin_indices[node_id], nodes.node_output_pin_indices[node_id] };
      for (const auto& in_pin : node_pins | std::views::join) {
        // Remove pin data
        pins.pin_node_indices.erase(pins.pin_node_indices.begin() + in_pin);
        pins.pin_indices_within_node.erase(pins.pin_indices_within_node.begin() + in_pin);
        pins.pin_positions.erase(pins.pin_positions.begin() + in_pin);
        pins.pin_radii.erase(pins.pin_radii.begin() + in_pin);
        pins.pin_node_relative_positions.erase(pins.pin_node_relative_positions.begin() + in_pin);
        pins.pin_colors.erase(pins.pin_colors.begin() + in_pin);
        pins.interactables.erase(pins.interactables.begin() + in_pin);
        pins.pin_states.erase(pins.pin_states.begin() + in_pin);
        pins.pin_types.erase(pins.pin_types.begin() + in_pin);

        /// all links into this pin
        for (natural_t link_idx = 0; link_idx < links.link_start_pin_indices.size(); ++link_idx) {
          natural_t start_pin = links.link_start_pin_indices[link_idx];
          natural_t end_pin = links.link_end_pin_indices[link_idx];

          if (start_pin == in_pin || end_pin == in_pin) {
            links.link_start_pin_indices.erase(links.link_start_pin_indices.begin() + link_idx);
            links.link_end_pin_indices.erase(links.link_end_pin_indices.begin() + link_idx);

            // Adjust all subsequent link indices
            for (natural_t i = 0; i < links.link_start_pin_indices.size(); ++i) {
              if (links.link_start_pin_indices[i] > start_pin) {
                --links.link_start_pin_indices[i];
              }
              if (links.link_end_pin_indices[i] > end_pin) {
                --links.link_end_pin_indices[i];
              }
            }

            --link_idx;
          }
        }
      }

      nodes.node_names.erase(nodes.node_names.begin() + node_id);
      nodes.node_positions.erase(nodes.node_positions.begin() + node_id);
      nodes.node_sizes.erase(nodes.node_sizes.begin() + node_id);
      nodes.node_header_colors.erase(nodes.node_header_colors.begin() + node_id);
      nodes.node_body_colors.erase(nodes.node_body_colors.begin() + node_id);
      nodes.node_input_pin_indices.erase(nodes.node_input_pin_indices.begin() + node_id);
      nodes.node_output_pin_indices.erase(nodes.node_output_pin_indices.begin() + node_id);
      nodes.interactables.erase(nodes.interactables.begin() + node_id);
      nodes.node_states.erase(nodes.node_states.begin() + node_id);

      nodes.global_node_positions.erase(nodes.global_node_positions.begin() + node_id);
      nodes.full_node_maxs.erase(nodes.full_node_maxs.begin() + node_id);
      nodes.node_header_ends.erase(nodes.node_header_ends.begin() + node_id);
      nodes.node_body_begins.erase(nodes.node_body_begins.begin() + node_id);
    }

    void node_editor_canvas_node::clear_all_nodes() {
      nodes = node_data{};
      pins = pin_data{};
      links = link_data{};
    }

    void node_editor_canvas_node::connect_node_pins(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx) {
      natural_t from_name_hash = FNV(from_node);
      natural_t to_name_hash = FNV(to_node);

      auto from_itr = std::ranges::find_if(nodes.node_names, [from_name_hash](const auto& n) { return FNV(n) == from_name_hash; });
      if (from_itr == nodes.node_names.end()) {
        CORE_LOG_ERROR("From node '{}' not found in node editor!", from_node);
        return;
      }

      auto to_itr = std::ranges::find_if(nodes.node_names, [to_name_hash](const auto& n) { return FNV(n) == to_name_hash; });
      if (to_itr == nodes.node_names.end()) {
        CORE_LOG_ERROR("To node '{}' not found in node editor!", to_node);
        return;
      }

      natural_t from_node_id = from_itr - nodes.node_names.begin();
      natural_t to_node_id = to_itr - nodes.node_names.begin();
      connect_node_pins(from_node_id, from_pin_idx, to_node_id, to_pin_idx);
    }

    void node_editor_canvas_node::connect_node_pins(natural_t from_node_id, uint8_t from_pin_idx, natural_t to_node_id, uint8_t to_pin_idx) {
      natural_t start_pin_id = nodes.node_output_pin_indices[from_node_id][from_pin_idx];
      natural_t end_pin_id = nodes.node_input_pin_indices[to_node_id][to_pin_idx];
      glm::vec4 link_color = { colors::kBasicNodeLinkColor.x, colors::kBasicNodeLinkColor.y, colors::kBasicNodeLinkColor.z, colors::kBasicNodeLinkColor.w };
      links.create(start_pin_id, end_pin_id, link_color);
      pins.pin_states[start_pin_id] = pin_data::pin_state::LINKED;
      pins.pin_states[end_pin_id] = pin_data::pin_state::LINKED;
    }

    void node_editor_canvas_node::on_prepare_render() {
    }

    void node_editor_canvas_node::on_render_node_body() {
      canvas_position = { ImGui::GetCurrentWindow()->Pos.x, ImGui::GetCurrentWindow()->Pos.y };
      canvas_size = { ImGui::GetCurrentWindow()->Size.x * zoom_level, ImGui::GetCurrentWindow()->Size.y * zoom_level };

      // draw_grid_lines();

      ImVec2 base_position = ImGui::GetCursorScreenPos();
      canvas_base_position = { base_position.x, base_position.y };

      if (ImGui::IsWindowHovered()) {
        if (ImGui::GetIO().MouseWheel > 0.f) {
          zoom_level += 0.1f;
        } else if (ImGui::GetIO().MouseWheel < 0.f) {
          zoom_level = std::max(0.1f, zoom_level - 0.1f);
        }
      }

      for (natural_t node_id = 0; node_id < nodes.node_names.size(); ++node_id) {
        update_node_state(node_id);
      }

      bool link_open = currently_open_link.has_value();
      bool moving_link_dest = dragging_link.has_value();

      bool link_mouse_button_down = ImGui::IsMouseDown(ImGuiMouseButton_Right);
      if ((link_open || moving_link_dest) && link_mouse_button_down) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
      }

      bool no_nodes_hovered = std::ranges::none_of(nodes.interactables, [](const auto& inter) { return inter.hovered; });
      if (link_open && no_nodes_hovered && !link_mouse_button_down) {
        create_node = true;
      } else if (link_open && !link_mouse_button_down) {
        close_open_link = true;
      }

      if (moving_link_dest && no_nodes_hovered && !link_mouse_button_down) {
        drop_dragged_link = true;
      } else if (moving_link_dest && !link_mouse_button_down) {
        dragging_link = std::nullopt;
      }

      for (natural_t node_id = 0; node_id < nodes.node_names.size(); ++node_id) {
        render_node(node_id);
      }

      for (natural_t link_id = 0; link_id < links.link_start_pin_indices.size(); ++link_id) {
        render_link(link_id);
      }

      if (currently_open_link.has_value()) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        natural_t start_pin_id = currently_open_link->pin_id;

        glm::vec2 start_pos = pins.pin_positions[start_pin_id];
        glm::vec2 end_pos = { mouse_pos.x, mouse_pos.y };

        ImGui::GetWindowDrawList()->AddBezierQuadratic(
          ImVec2(start_pos.x, start_pos.y),
          ImVec2((start_pos.x + end_pos.x) * 0.5f, (start_pos.y + end_pos.y) * 0.5f + 50.f),
          ImVec2(end_pos.x, end_pos.y),
          colors::rgba_to_hex(colors::kDataLinkColor),
          3.0f
        );
      } else if (dragging_link.has_value()) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        natural_t start_pin_id = dragging_link->pin_id;

        glm::vec2 start_pos = pins.pin_positions[start_pin_id];
        glm::vec2 end_pos = { mouse_pos.x, mouse_pos.y };

        ImGui::GetWindowDrawList()->AddBezierQuadratic(
          ImVec2(start_pos.x, start_pos.y),
          ImVec2((start_pos.x + end_pos.x) * 0.5f, (start_pos.y + end_pos.y) * 0.5f + 50.f),
          ImVec2(end_pos.x, end_pos.y),
          colors::rgba_to_hex(colors::kDataLinkColor),
          3.0f
        );
      }
    }

    void node_editor_canvas_node::on_render_end() {
      if (create_node) {
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        natural_t node_id = create_single_node("New Node", 2, 2);
        nodes.node_positions[node_id] = glm::vec2(mouse_pos.x - canvas_base_position.x, mouse_pos.y - canvas_base_position.y);

        // add link from open pin to new node's first input pin
        natural_t start_pin_id = currently_open_link->pin_id;
        natural_t end_pin_id = nodes.node_input_pin_indices[node_id][0];
        glm::vec4 link_color = { colors::kBasicNodeLinkColor.x, colors::kBasicNodeLinkColor.y, colors::kBasicNodeLinkColor.z, colors::kBasicNodeLinkColor.w };
        links.create(start_pin_id, end_pin_id, link_color);
        pins.pin_states[start_pin_id] = pin_data::pin_state::LINKED;
        pins.pin_states[end_pin_id] = pin_data::pin_state::LINKED;
      }

      if (close_open_link) {
        auto pin_itr = std::ranges::find_if(pins.interactables, [](const auto& state) { return state.hovered; });
        if (pin_itr != pins.interactables.end()) {
          natural_t end_pin_id = pin_itr - pins.interactables.begin();

          bool can_link =
            pins.pin_states[end_pin_id] != pin_data::pin_state::LINKED &&
            pins.pin_types[end_pin_id] == pin_data::pin_type::INPUT;

          if (can_link) {
            glm::vec4 link_color = { colors::kBasicNodeLinkColor.x, colors::kBasicNodeLinkColor.y, colors::kBasicNodeLinkColor.z, colors::kBasicNodeLinkColor.w };
            links.create(currently_open_link->pin_id, end_pin_id, link_color);
            pins.pin_states[currently_open_link->pin_id] = pin_data::pin_state::LINKED;
            pins.pin_states[end_pin_id] = pin_data::pin_state::LINKED;
          }
        }
      }

      if (drop_dragged_link) {
        natural_t link_id = -1;
        for (natural_t i = 0; i < links.link_start_pin_indices.size(); ++i) {
          if (links.link_start_pin_indices[i] == dragging_link->pin_id || links.link_end_pin_indices[i] == dragging_link->pin_id) {
            link_id = i;
            break;
          }
        }
        if (link_id != static_cast<natural_t>(-1)) {
          links.link_start_pin_indices.erase(links.link_start_pin_indices.begin() + link_id);
          links.link_end_pin_indices.erase(links.link_end_pin_indices.begin() + link_id);
          for (size_t i = 0; i < links.kNumBezierPoints; ++i) {
            links.link_bezier_points[i].erase(links.link_bezier_points[i].begin() + link_id);
          }
          links.link_colors.erase(links.link_colors.begin() + link_id);
        }
      }

      if (create_node || close_open_link) {
        create_node = false;
        creating_node = false;
        close_open_link = false;
        currently_open_link = std::nullopt;
      }

      if (drop_dragged_link) {
        drop_dragged_link = false;
        dragging_link = std::nullopt;
      }
    }

    void node_editor_canvas_node::draw_grid_lines() {
      static constexpr float grid_step = 50.0f;
      static constexpr float major_grid_step = grid_step * 5.0f;
      static constexpr glm::vec4 grid_color = glm::vec4(0.2f, 0.2f, 0.2f, 0.4f);
      static constexpr glm::vec4 major_grid_color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
      static constexpr float grid_thickness = 1.0f;
      static constexpr float major_thickness = 2.0f;

      // ImVec4 im_grid_color = testing_colors::editor::kGridColor;
      // ImVec4 im_major_grid_color = testing_colors::editor::kMajorGridColor;

      ImVec2 canvas_size = ImGui::GetContentRegionAvail();
      ImVec2 canvas_min = ImGui::GetCursorScreenPos();
      ImVec2 canvas_max = ImVec2(canvas_min.x + canvas_size.x, canvas_min.y + canvas_size.y);

      // Draw vertical grid lines
      for (float x = fmodf(canvas_min.x, grid_step); x < canvas_size.x; x += grid_step) {
        float distance_to_major = fmodf(x, major_grid_step);
        float proximity = 1.0f - glm::min(distance_to_major, major_grid_step - distance_to_major) / (major_grid_step * 0.5f);

        float alpha = glm::mix(grid_color.a, major_grid_color.a, proximity);
        float thickness = glm::mix(grid_thickness, major_thickness, proximity);

        ImVec4 interpolated_color = { grid_color.r, grid_color.g, grid_color.b, alpha };
        ImU32 line_color = ImGui::GetColorU32(interpolated_color);

        ImGui::GetWindowDrawList()->AddLine(
          ImVec2(canvas_min.x + x, canvas_min.y),
          ImVec2(canvas_min.x + x, canvas_max.y),
          line_color, thickness
        );
      }

      // Draw horizontal grid lines
      for (float y = fmodf(canvas_min.y, grid_step); y < canvas_size.y; y += grid_step) {
        float distance_to_major = fmodf(y, major_grid_step);
        float proximity = 1.0f - glm::min(distance_to_major, major_grid_step - distance_to_major) / (major_grid_step * 0.5f);

        float alpha = glm::mix(grid_color.a, major_grid_color.a, proximity);
        float thickness = glm::mix(grid_thickness, major_thickness, proximity);

        ImVec4 interpolated_color = { grid_color.r, grid_color.g, grid_color.b, alpha };
        ImU32 line_color = ImGui::GetColorU32(interpolated_color);

        ImGui::GetWindowDrawList()->AddLine(
          ImVec2(canvas_min.x, canvas_min.y + y),
          ImVec2(canvas_max.x, canvas_min.y + y),
          line_color, thickness
        );
      }
    }

    void node_editor_canvas_node::render_node(natural_t node_id) {
      auto& node_name = nodes.node_names[node_id];

      std::string item_tag = std::format("##node_{}", node_name);
      const ImGuiID id = ImGui::GetCurrentWindow()->GetID(item_tag.c_str());
      ImGui::PushID(node_name.c_str());
      ImGui::BeginGroup();

      if (snap_to_grid) {
        // /// snap to grid
        // glm::vec2 snapped_pos;
        // snapped_pos.x = std::round(nodes.node_positions[node_id].x / grid_line_spacing) * grid_line_spacing;
        // snapped_pos.y = std::round(nodes.node_positions[node_id].y / grid_line_spacing) * grid_line_spacing;
        // nodes.node_positions[node_id] = snapped_pos;
      }

      nodes.global_node_positions[node_id] = { canvas_base_position.x + nodes.node_positions[node_id].x, canvas_base_position.y + nodes.node_positions[node_id].y };
      nodes.full_node_maxs[node_id] = nodes.global_node_positions[node_id] + nodes.node_sizes[node_id];

      nodes.node_header_ends[node_id] = { nodes.full_node_maxs[node_id].x, nodes.global_node_positions[node_id].y + ImGui::GetFrameHeight() };
      nodes.node_body_begins[node_id] = { nodes.global_node_positions[node_id].x, nodes.node_header_ends[node_id].y };

      const glm::vec2& position = nodes.global_node_positions[node_id];
      const glm::vec2& size = nodes.node_sizes[node_id];
      const auto& state = nodes.interactables[node_id];

      ImRect rect{ { position.x, position.y }, { position.x + size.x, position.y + size.y } };
      ImRect titlebar_rect{ rect.Min, { position.x + size.x, position.y + ImGui::GetFrameHeight() } };
      ImRect body_rect{ { position.x, position.y + ImGui::GetFrameHeight() }, rect.Max };

      auto* draw_list = ImGui::GetWindowDrawList();

      /// bg
      auto bg_col = colors::kNodeBodyColor;
      if (state.hovered) {
        bg_col = colors::kNodeCardStrokeColor;
      } else {
        bg_col = colors::kNodeBodyColor;
      }
      draw_list->AddRectFilled(rect.Min, rect.Max, colors::rgba_to_hex(bg_col), 4.0f);

      /// title bar
      std::string display_name = calculate_display_text(node_name, (titlebar_rect.Max.x - titlebar_rect.Min.x) - 16.f);
      std::string node_id_str = std::format("[{}]", node_id);

      /// text and other data goes in right and left corner of title bar
      ImVec2 text_pos = ImVec2{ nodes.global_node_positions[node_id].x + 8.0f, nodes.global_node_positions[node_id].y + 2.f };
      ImVec2 opp_text_pos = ImVec2{ titlebar_rect.Max.x - 24.f, text_pos.y };

      draw_list->AddRectFilled(titlebar_rect.Min, titlebar_rect.Max, colors::rgba_to_hex(colors::kNodeHeaderColor), 4.0f);
      draw_list->AddText(text_pos, (colors::rgba_to_hex(colors::kNodeTitleTextColor)), display_name.c_str());
      draw_list->AddText(opp_text_pos, colors::rgba_to_hex(colors::kNodeTitleTextColor), node_id_str.c_str());

      /// body
      draw_list->AddRectFilled(body_rect.Min, body_rect.Max, colors::rgba_to_hex(colors::kNodeBodyColor), 4.0f);

      const auto& input_pins = nodes.node_input_pin_indices[node_id];
      const auto& output_pins = nodes.node_output_pin_indices[node_id];
      auto pins = { input_pins, output_pins };
      for (const auto& p : pins | std::views::join) {
        render_pin(p);
      }

      /// value display
      size_t num_input_pins = input_pins.size();
      size_t num_output_pins = output_pins.size();

      float inner_body_in_pin_width_adj = 4.f;
      float inner_body_out_pin_width_adj = 4.f;

      inner_body_in_pin_width_adj += num_input_pins > 0 ? 16.f : 0.f;
      inner_body_out_pin_width_adj += num_output_pins > 0 ? 16.f : 0.f;

      ImRect body_without_pins_rect = {
        { body_rect.Min.x + inner_body_in_pin_width_adj, body_rect.Min.y + 8.f },
        { body_rect.Max.x - inner_body_out_pin_width_adj, body_rect.Max.y - 8.f }
      };

      ImGui::SetCursorPos(ImVec2{ body_without_pins_rect.Min.x - ImGui::GetWindowPos().x, body_without_pins_rect.Min.y - ImGui::GetWindowPos().y });
      editor->render_node_body(node_id, body_without_pins_rect);

      /// frame
      draw_list->AddRect(rect.Min, rect.Max, colors::rgba_to_hex(colors::kNodeOutlineColor), 4.0f, ImDrawFlags_None, 2.0f);

      ImGui::EndGroup();
      ImGui::PopID();
    }

    void node_editor_canvas_node::render_pin(natural_t pin_id) {
      ImVec4 ig_pin_color = ImVec4(1.f, 0.f, 0.f, 1.f);
      ImVec2 pin_pos = ImVec2{ pins.pin_positions[pin_id].x, pins.pin_positions[pin_id].y };

      float radius = glm::clamp(pins.pin_radii[pin_id], 3.3f, 7.0f);
      ImGui::GetWindowDrawList()->AddCircleFilled(pin_pos, radius, ImGui::GetColorU32(ig_pin_color));
    }

    void node_editor_canvas_node::render_link(natural_t link_id) {
      const auto& start_pin = pins.pin_positions[links.link_start_pin_indices[link_id]];
      const auto& end_pin = pins.pin_positions[links.link_end_pin_indices[link_id]];

      const auto& bez1 = links.link_bezier_points[0][link_id];
      // const auto& bez2 = links.link_bezier_points[1][link_id];

      ImGui::GetWindowDrawList()->AddBezierCubic(
        ImVec2(start_pin.x, start_pin.y),
        ImVec2(start_pin.x + 50.f, start_pin.y),
        ImVec2(end_pin.x - 50.f, end_pin.y),
        ImVec2(end_pin.x, end_pin.y),
        colors::rgba_to_hex(colors::kDataLinkColor),
        3.0f
      );
    }

    void node_editor_canvas_node::update_node_state(natural_t node_id) {
      const auto& node_name = nodes.node_names[node_id];

      nodes.global_node_positions[node_id] = { canvas_base_position.x + nodes.node_positions[node_id].x, canvas_base_position.y + nodes.node_positions[node_id].y };
      nodes.full_node_maxs[node_id] = nodes.global_node_positions[node_id] + nodes.node_sizes[node_id];  // * zoom_level;

      nodes.node_header_ends[node_id] = { nodes.full_node_maxs[node_id].x, nodes.global_node_positions[node_id].y + ImGui::GetFrameHeight() };
      nodes.node_body_begins[node_id] = { nodes.global_node_positions[node_id].x, nodes.node_header_ends[node_id].y };

      size_t num_input_pins = nodes.node_input_pin_indices[node_id].size();
      size_t num_output_pins = nodes.node_output_pin_indices[node_id].size();

      float node_height_without_title = nodes.node_sizes[node_id].y - ImGui::GetFrameHeight();

      float input_pin_spacing = node_height_without_title / (num_input_pins + 1);
      float output_pin_spacing = node_height_without_title / (num_output_pins + 1);

      constexpr float x_padding = 8.f;

      glm::vec2 global_node_pos = nodes.global_node_positions[node_id];
      float global_node_include_title = nodes.node_header_ends[node_id].y;

      for (auto& p : nodes.node_input_pin_indices[node_id]) {
        float y_pos = global_node_include_title + input_pin_spacing * (pins.pin_indices_within_node[p] + 1);
        pins.pin_node_relative_positions[p] = glm::ivec2(x_padding, y_pos - global_node_include_title);
        pins.pin_positions[p] = glm::ivec2(global_node_pos.x + x_padding, y_pos);
      }

      for (auto& p : nodes.node_output_pin_indices[node_id]) {
        float y_pos = global_node_include_title + output_pin_spacing * (pins.pin_indices_within_node[p] + 1);
        pins.pin_node_relative_positions[p] = glm::ivec2(nodes.node_sizes[node_id].x - x_padding, y_pos - global_node_include_title);
        pins.pin_positions[p] = glm::ivec2(global_node_pos.x + nodes.node_sizes[node_id].x - x_padding, y_pos);
      }

      auto id = ImGui::GetID(node_name.data());
      ImVec2 node_size = ImVec2(nodes.node_sizes[node_id].x, nodes.node_sizes[node_id].y);
      ImVec2 item_size = ImGui::CalcItemSize(node_size, 0.0f, 0.0f);

      glm::vec2 node_global_pos = nodes.global_node_positions[node_id];
      ImRect rect{ { node_global_pos.x, node_global_pos.y }, { node_global_pos.x + nodes.node_sizes[node_id].x, node_global_pos.y + nodes.node_sizes[node_id].y } };

      ImGui::ItemSize(item_size);
      ImGui::ItemAdd(rect, id, nullptr, ImGuiItemFlags_None);

      bool previously_stretching = nodes.node_states[node_id] == node_data::node_state::RESIZING;
      bool previously_dragging = nodes.node_states[node_id] == node_data::node_state::DRAGGING;

      bool continue_stretching = previously_stretching && ImGui::IsMouseDown(ImGuiMouseButton_Left);
      bool continue_dragging = previously_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left);

      /// reset here, we can fully recalculate below
      nodes.node_states[node_id] = node_data::node_state::NORMAL;

      ImRect titlebar_rect{ rect.Min, { nodes.node_header_ends[node_id].x, nodes.node_header_ends[node_id].y } };
      ImRect resize_grip_rect = {
        nodes.full_node_maxs[node_id].x - kNodeResizeGripSize - kNodeResizeGripPadding, nodes.full_node_maxs[node_id].y - kNodeResizeGripSize - kNodeResizeGripPadding,
        nodes.full_node_maxs[node_id].x + kNodeResizeGripPadding, nodes.full_node_maxs[node_id].y + kNodeResizeGripPadding
      };

      bool just_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
      bool mouse_in_titlebar = ImGui::IsMouseHoveringRect(titlebar_rect.Min, titlebar_rect.Max);
      bool mouse_down_in_titlebar = mouse_in_titlebar && just_clicked;
      bool just_selected = mouse_down_in_titlebar;
      if (just_selected) {
        selected_node_id = node_id;
      }

      nodes.interactables[node_id].hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);

      bool hovering_boundary = ImGui::IsMouseHoveringRect(resize_grip_rect.Min, resize_grip_rect.Max);
      bool mouse_down_on_boundary = hovering_boundary && just_clicked;

      ImGuiMouseCursor cursor_to_use = ImGuiMouseCursor_Arrow;
      if (hovering_boundary) {
        if (ImGui::GetIO().MousePos.x > nodes.full_node_maxs[node_id].x - kNodeResizeGripSize) {
          cursor_to_use = ImGuiMouseCursor_ResizeEW;
        } else if (ImGui::GetIO().MousePos.y > nodes.full_node_maxs[node_id].y - kNodeResizeGripSize) {
          cursor_to_use = ImGuiMouseCursor_ResizeNS;
        }

        /// if EW || NS check if both
        if (cursor_to_use != ImGuiMouseCursor_Arrow) {
          if (ImGui::GetIO().MousePos.x > nodes.full_node_maxs[node_id].x - kNodeResizeGripSize &&
              ImGui::GetIO().MousePos.y > nodes.full_node_maxs[node_id].y - kNodeResizeGripSize) {
            cursor_to_use = ImGuiMouseCursor_ResizeNWSE;
          }
        }

        ImGui::SetMouseCursor(cursor_to_use);
      }

      bool is_stretching = mouse_down_on_boundary || continue_stretching;
      if (is_stretching) {
        nodes.node_states[node_id] = node_data::node_state::RESIZING;
      }

      bool is_dragging = mouse_down_in_titlebar || continue_dragging;
      if (is_dragging) {
        nodes.node_states[node_id] = node_data::node_state::DRAGGING;
      }

      if (nodes.node_states[node_id] == node_data::node_state::DRAGGING) {
        nodes.node_positions[node_id].x += ImGui::GetIO().MouseDelta.x;
        nodes.node_positions[node_id].y += ImGui::GetIO().MouseDelta.y;

      } else if (nodes.node_states[node_id] == node_data::node_state::RESIZING) {
        nodes.node_sizes[node_id].x += ImGui::GetIO().MouseDelta.x;
        nodes.node_sizes[node_id].y += ImGui::GetIO().MouseDelta.y;

        if (nodes.node_sizes[node_id].x < kMinNodeWidth) {
          nodes.node_sizes[node_id].x = kMinNodeWidth;
        }
        if (nodes.node_sizes[node_id].y < kMinNodeHeight) {
          nodes.node_sizes[node_id].y = kMinNodeHeight;
        }
      }

      auto this_node_pins = { nodes.node_input_pin_indices[node_id], nodes.node_output_pin_indices[node_id] };
      for (auto& p : this_node_pins | std::views::join) {
        update_pin_state(p);
      }
    }

    void node_editor_canvas_node::update_pin_state(natural_t pin_id) {
      auto& position = pins.pin_positions[pin_id];
      // auto& size = glm::vec2(12.0f, 12.0f);
      auto& state = pins.interactables[pin_id];

      state.hovered = ImGui::IsMouseHoveringRect(
        ImVec2{ position.x - 6.0f, position.y - 6.0f },
        ImVec2{ position.x + 6.0f, position.y + 6.0f }
      );

      state.clicked = state.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
      state.right_clicked = state.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

      bool linked = pins.pin_states[pin_id] == pin_data::pin_state::LINKED;
      bool source_link = pins.pin_types[pin_id] == pin_data::pin_type::OUTPUT;
      bool dest_link = pins.pin_types[pin_id] == pin_data::pin_type::INPUT;

      if (state.right_clicked) {
        /// if right-click an input node then pick up the end of the link
        if (dest_link) {
          pins.pin_states[pin_id] = pin_data::pin_state::DRAGGING_LINK;
          dragging_link = { pin_id, pins.pin_types[pin_id] };
        }
        /// if right-click an output node then start a new link from there
        else if (source_link) {
          pins.pin_states[pin_id] = pin_data::pin_state::STARTING_LINK;
          currently_open_link = open_link{ pin_id, pins.pin_types[pin_id] };
        }
      }

      /// if parent node is too small, then make pin smaller too
      /// if pin is small and node is big, make pin bigger
      /// also have to take into account number of pins on the node
      float parent_node_size_y = nodes.node_sizes[pins.pin_node_indices[pin_id]].y;
      float target_radius = glm::clamp(parent_node_size_y / 15.f, 2.2f, 10.f);
      float radius_diff = target_radius - pins.pin_radii[pin_id];

      natural_t parent_node_in_pins = nodes.node_input_pin_indices[pins.pin_node_indices[pin_id]].size();
      natural_t parent_node_out_pins = nodes.node_output_pin_indices[pins.pin_node_indices[pin_id]].size();
      natural_t num_other_pins = pins.pin_types[pin_id] == pin_data::pin_type::INPUT ? parent_node_out_pins : parent_node_in_pins;
      natural_t other_pin_factor = num_other_pins > 4 ? num_other_pins - 4 : 0;

      target_radius = glm::clamp(parent_node_size_y / (15.f + other_pin_factor * 2.f), 2.2f, 10.f);
      radius_diff = target_radius - pins.pin_radii[pin_id];

      pins.pin_radii[pin_id] += radius_diff * 0.1f;
    }

  }  // namespace ui
}  // namespace other