/**
 * \file theme/node_editor_style.hpp
 **/
#ifndef OTHER_UI_THEME_NODE_EDITOR_STYLE_HPP
#define OTHER_UI_THEME_NODE_EDITOR_STYLE_HPP

#include <glm/glm.hpp>

#include "theme/colors.hpp"

namespace other {

  struct pin_style {
    glm::vec4 color = ui::colors::kDataLinkColor;
    uint8_t shape = 0;  //< 0 = circle, 1 = square
    float radius = 5.f;
  };

  struct link_style {
    glm::vec4 color = ui::colors::kBasicNodeLinkColor;

    float thickness = 2.5f;
    float curvature = 50.f;  //< 0 = straight line, > 0 = horizontal bezier
    float tangent = 50.f;

    bool arrow_head = false;

    std::string_view label = {};
  };

  struct node_canvas_style {
    glm::vec4 background = ui::colors::kNodeEditorBackground;
    glm::vec4 grid = { 0.2f, 0.2f, 0.2f, 0.4f };
    glm::vec4 grid_major = { 0.8f, 0.8f, 0.8f, 0.35f };
    glm::vec4 node_body = ui::colors::kNodeBodyColor;
    glm::vec4 node_text = ui::colors::kNodeTextColor;
    glm::vec4 node_title_text = ui::colors::kNodeTitleTextColor;
    glm::vec4 node_outline = ui::colors::kNodeOutlineColor;
    glm::vec4 node_outline_selected = ui::colors::kOutlineFocus;
    glm::vec4 node_outline_hovered = ui::colors::kNodeCardStrokeColor;
    glm::vec4 link_invalid = ui::colors::kError;

    float node_rounding = 4.f;
    float node_padding = 8.f;
    float pin_label_gap = 6.f;
    float pin_spacing = 20.f;
    float pin_radius = 5.f;
    float header_height_scale = 1.2f;   ///< × ImGui::GetFrameHeight()
    float lod_body_cutoff_zoom = 0.4f;  ///< below this, bodies collapse to title bars
    float min_zoom = 0.1f;
    float max_zoom = 2.5f;

    pin_style input_pin_style;
    pin_style output_pin_style;

    link_style default_link_style;
    std::map<natural_t, link_style> link_styles;
  };

}  // namespace other

#endif  // OTHER_UI_THEME_NODE_EDITOR_STYLE_HPP