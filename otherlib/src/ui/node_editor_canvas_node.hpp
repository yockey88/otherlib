/**
 * \file ui/node_editor_canvas_node.hpp
 **/
#ifndef OTHERLIB_UI_NODE_EDITOR_NODES_NODE_EDITOR_CANVAS_NODE_HPP
#define OTHERLIB_UI_NODE_EDITOR_NODES_NODE_EDITOR_CANVAS_NODE_HPP

#include "core/defines.hpp"

#include "renderer/ui/ui_interactable.hpp"
#include "renderer/ui/ui_node.hpp"

#include "imgui.h"

namespace other {
  namespace ui {

    struct node_editor;

    struct node_editor_canvas_node : public ui_node {
      constexpr static float kNodeResizeGripPadding = 25.f;
      constexpr static float kNodeResizeGripSize = 10.f;
      constexpr static float kMinNodeSize = 50.f;
      struct node_data {
        enum node_state {
          NORMAL,
          DRAGGING,
          RESIZING,
          SELECTED,
        };

        std::vector<std::string> node_names;
        std::vector<glm::vec2> node_positions;
        std::vector<glm::vec2> node_sizes;
        std::vector<glm::vec4> node_header_colors;
        std::vector<glm::vec4> node_body_colors;
        std::vector<std::vector<natural_t>> node_input_pin_indices;
        std::vector<std::vector<natural_t>> node_output_pin_indices;
        std::vector<ui_interactable> interactables;
        std::vector<node_state> node_states;

        std::vector<glm::vec2> global_node_positions;
        std::vector<glm::vec2> full_node_maxs;
        std::vector<glm::vec2> node_header_ends;
        std::vector<glm::vec2> node_body_begins;

        natural_t create(const std::string_view node_name, const glm::vec2& position, const glm::vec2& size, const glm::vec4& header_color, const glm::vec4& body_color);
      };
      struct pin_data {
        enum pin_state {
          NORMAL,
          STARTING_LINK,
          DRAGGING_LINK,
          LINKED
        };
        enum pin_type {
          INPUT,
          OUTPUT
        };
        std::vector<natural_t> pin_node_indices;
        std::vector<natural_t> pin_indices_within_node;
        std::vector<glm::vec2> pin_positions;
        std::vector<float> pin_radii;
        std::vector<glm::vec2> pin_node_relative_positions;
        std::vector<glm::vec4> pin_colors;
        std::vector<ui_interactable> interactables;
        std::vector<pin_state> pin_states;
        std::vector<pin_type> pin_types;

        natural_t create(natural_t node_id, natural_t pin_index, pin_type type);
      };
      struct link_data {
        std::vector<natural_t> link_start_pin_indices;
        std::vector<natural_t> link_end_pin_indices;

        constexpr static size_t kNumBezierPoints = 1;
        std::vector<glm::vec2> link_bezier_points[kNumBezierPoints];
        std::vector<glm::vec4> link_colors;

        natural_t create(natural_t start_pin_idx, natural_t end_pin_idx, const glm::vec4& color);
      };

      node_editor_canvas_node(node_editor* parent)
          : ui_node((ui_window*)parent, "Node Editor Canvas", glm::vec2(0, 0), ImGuiChildFlags_Borders /* | ImGuiChildFlags_FrameStyle */, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar), editor(parent) {}
      virtual ~node_editor_canvas_node() = default;

      natural_t create_single_node(const std::string_view node_name, uint8_t input_pins, uint8_t output_pins);

      void on_prepare_render() override;
      void on_render_node_body() override;
      void on_render_end() override;

      void draw_grid_lines();

      glm::vec2 canvas_position{ 0.f, 0.f };
      glm::vec2 canvas_size{ 800.f, 600.f };
      glm::vec2 canvas_base_position{ 0.f, 0.f };

     private:
      node_data nodes;
      pin_data pins;
      link_data links;

      struct open_link {
        natural_t pin_id = static_cast<natural_t>(-1);
        pin_data::pin_type type;
      };
      opt<open_link> currently_open_link = std::nullopt;
      bool create_node = false;
      bool creating_node = false;
      bool close_open_link = false;

      opt<open_link> dragging_link = std::nullopt;
      bool drop_dragged_link = false;

      node_editor* editor = nullptr;

      bool snap_to_grid = true;

      float grid_line_spacing = 10.f;
      float major_grid_line_spacing = 50.f;

      glm::vec4 grid_color = glm::vec4(0.2f, 0.2f, 0.2f, 0.4f);
      glm::vec4 major_grid_color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

      natural_t selected_node_id = static_cast<natural_t>(-1);

      void render_nodes_and_pins();
      void render_node(natural_t node_id);
      void render_pin(natural_t pin_id);
      void render_link(natural_t link_id);

      void update_node_state(natural_t node_id);
      void update_pin_state(natural_t pin_id);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_NODE_EDITOR_NODES_NODE_EDITOR_CANVAS_NODE_HPP
