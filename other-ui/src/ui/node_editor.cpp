/**
 * \file ui/node_editor.cpp
 **/
#include "ui/node_editor.hpp"

#include <imgui/imgui.h>

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "theme/colors.hpp"
#include "ui/node_editor_math.hpp"

namespace other {

  void node_editor::begin(const std::string_view title, const glm::vec2& size) {
    OTHER_ASSERT(!frame_open, "node_editor::begin('{}') called twice without end", title);

    frame_open = true;
    ++frame_index;

    submitted_nodes.clear();
    pins.clear();
    links.clear();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui::colors::rgba_to_imvec4(style.background));
    ImGui::BeginChild(std::format("##node_editor_{}", title).c_str(), { size.x, size.y },
                      ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    view_transform.window_origin = { origin.x, origin.y };

    const float z = view_transform.zoom;
    ImGui::SetWindowFontScale(z);

    const ImGuiStyle& s = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { s.FramePadding.x * z, s.FramePadding.y * z });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { s.ItemSpacing.x * z, s.ItemSpacing.y * z });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { s.ItemInnerSpacing.x * z, s.ItemInnerSpacing.y * z });
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, s.IndentSpacing * z);

    draw_list = ImGui::GetWindowDrawList();
    splitter = &draw_list->_Splitter;
    splitter->Split(draw_list, 2);
    splitter->SetCurrentChannel(draw_list, 0);

    draw_grid();
    splitter->SetCurrentChannel(draw_list, 1);
  }

  void node_editor::end() {
    OTHER_ASSERT(frame_open, "node_editor::end called without begin");
    OTHER_ASSERT(draw_list != nullptr, "node_editor::end called but draw_list is null");
    OTHER_ASSERT(splitter != nullptr, "node_editor::end called but splitter is null");
    OTHER_ASSERT(current_node == kInvalidId, "node {} not closed before node_editor::end", current_node);

    process_interactions();

    splitter->SetCurrentChannel(draw_list, 0);
    // draw_links();

    splitter->Merge(draw_list);
    if (action == canvas_action::LINKING) {
      // const ImGuiIO& io = ImGui::GetIO();
      // draw_pending_link({ io.MousePos.x, io.MousePos.y });  ///< post-merge: on top
    }

    // mark_linked_pins();
    // process_interaction();
    // draw_links();
    // reorder_node_channels();
    // splitter_merge();

    // draw_overlays();

    ImGui::PopStyleVar(4);
    ImGui::SetWindowFontScale(1.f);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    splitter = nullptr;
    draw_list = nullptr;
    frame_open = false;
  }

  natural_t node_editor::begin_node(const std::string_view title, const glm::vec4& header_color) {
    // const natural_t node_id = node_uid(title);
    // OTHER_ASSERT(frame_open, "begin_node('{}') called outside node_editor::begin/end", title);
    // OTHER_ASSERT(current_node == kInvalidId, "begin_node('{}') called inside node {}", title, current_node);
    // OTHER_ASSERT(!submitted_nodes.contains(node_id),
    //              "node '{}' submitted twice this frame — node titles must be unique per canvas", title);

    // OTHER_ASSERT(draw_list != nullptr, "open_node called outside of begin/end");
    // OTHER_ASSERT(current_node == kInvalidId, "open_node called while node {} is already open", current_node);
    // current_node = node_id;

    // current_node = node_id;
    // submitted_nodes.emplace(node_id, submission_order.size());
    // submission_order.push_back(node_id);

    // auto [itr, inserted] = layouts.try_emplace(node_id);
    // if (inserted) {
    //   itr->second.position = spawn_position();
    //   z_order.push_back(node_id);
    // }
    // node_layout& layout = itr->second;
    // layout.last_submit_frame = frame_index;
    // layout.pin_offsets.clear();

    // const float zoom = view_transform.zoom;
    // const float pad = style.node_padding * zoom;
    // const float header_h = ImGui::GetFrameHeight();
    // const glm::vec2 rect_min = view_transform.to_screen(layout.position);
    // const glm::vec2 rect_max = rect_min + layout.size * zoom;

    // ImGui::PushID(title.data(), title.data() + title.size());

    // if (layout.size != glm::vec2{ 0.f, 0.f }) {
    //   const float rounding = style.node_rounding * zoom;
    //   draw_list->AddRectFilled(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_max.y }, ui::colors::rgba_to_hex(style.node_body), rounding);
    //   draw_list->AddRectFilled(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_min.y + header_h }, ui::colors::rgba_to_hex(header_color), rounding, ImDrawFlags_RoundCornersTop);
    //   draw_list->AddText(ImVec2{ rect_min.x + pad, rect_min.y + 0.5f * (header_h - ImGui::GetTextLineHeight()) }, ui::colors::rgba_to_hex(style.node_title_text), title.data(), title.data() + title.size());

    //   glm::vec2 rect_ext = rect_max - rect_min;
    //   ImGui::SetCursorScreenPos(ImVec2{ rect_min.x, rect_min.y });
    //   ImGui::InvisibleButton("##node_hit", ImVec2{ rect_ext.x, rect_ext.y }, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    // }

    // ImGui::SetCursorScreenPos(ImVec2{ rect_min.x + pad, rect_min.y + header_h + pad });
    // ImGui::BeginGroup();
    // return node_id;
    return 0;
  }

  void node_editor::end_node() {
    // OTHER_ASSERT(current_node != kInvalidId, "end_node without begin_node");
    // OTHER_ASSERT(current_pin == kInvalidId, "pin {} not closed before end_node", current_pin);
    // ImGui::EndGroup();

    // const ImVec2 content_min = ImGui::GetItemRectMin();
    // const ImVec2 content_max = ImGui::GetItemRectMax();
    // const float zoom = view_transform.zoom;
    // const float pad = style.node_padding * zoom;
    // const float header_h = ImGui::GetFrameHeight();

    // node_layout& layout = layouts.at(current_node);
    // layout.size = {
    //   (content_max.x - content_min.x + 2.f * pad) / zoom,
    //   (content_max.y - content_min.y + 2.f * pad + header_h) / zoom,
    // };

    // const glm::vec2 rect_min = view_transform.to_screen(layout.position);
    // const glm::vec2 rect_max = rect_min + layout.size * zoom;

    // for (pin_record& rec : pins) {
    //   if (rec.node_id != current_node) {
    //     continue;
    //   }
    //   rec.dot_screen_position = { rec.direction == pin_type::INPUT ? rect_min.x : rect_max.x, 0.5f * (rec.row_min_y + rec.row_max_y) };
    //   draw_list->AddCircleFilled(ImVec2{ rec.dot_screen_position.x, rec.dot_screen_position.y }, style.pin_radius * zoom, ui::colors::rgba_to_hex(rec.style.color));
    // }

    // const bool selected = contains_id(selection_nodes, current_node);
    // const bool hovered = hovered_node_id == current_node;  ///< last frame's hover
    // glm::vec4 outline = {};
    // if (selected) {
    //   outline = style.node_outline_selected;
    // } else if (hovered) {
    //   outline = style.node_outline_hovered;
    // } else {
    //   outline = style.node_outline;
    // }
    // draw_list->AddRect(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_max.y }, ui::colors::rgba_to_hex(outline),
    //                    style.node_rounding * zoom, 0, (selected ? 2.f : 1.f) * zoom);

    // ImGui::PopID();
    current_node = kInvalidId;
  }

  float node_editor::distance_to_link(const link_record& rec, const glm::vec2& point, float max_distance) const {
    // const glm::vec2 p0 = pins[pin_lookup.at(rec.from_pin)].dot_screen_position;
    // const glm::vec2 p3 = pins[pin_lookup.at(rec.to_pin)].dot_screen_position;
    // const float tangent = glm::min(rec.style.tangent, 0.5f * glm::distance(p0, p3)) * view_transform.zoom;
    // return distance_to_cubic(point, p0, { p0.x + tangent, p0.y }, { p3.x - tangent, p3.y }, p3, max_distance);
    return 0.f;
  }

  void node_editor::draw_grid() {
    OTHER_ASSERT(draw_list != nullptr, "draw_grid called outside of begin/end");

    /// take into account zoom
    float zoom_step = config.grid_step * view_transform.zoom;

    for (float x = std::fmod(view_transform.pan.x, zoom_step); x < ImGui::GetWindowContentRegionMax().x; x += zoom_step) {
      ImVec2 start = { view_transform.window_origin.x + x, view_transform.window_origin.y };
      ImVec2 end = { view_transform.window_origin.x + x, view_transform.window_origin.y + ImGui::GetWindowContentRegionMax().y };
      draw_list->AddLine(start, end, ui::colors::rgba_to_hex(style.grid));
    }
    for (float y = std::fmod(view_transform.pan.y, zoom_step); y < ImGui::GetWindowContentRegionMax().y; y += zoom_step) {
      ImVec2 start = { view_transform.window_origin.x, view_transform.window_origin.y + y };
      ImVec2 end = { view_transform.window_origin.x + ImGui::GetWindowContentRegionMax().x, view_transform.window_origin.y + y };
      draw_list->AddLine(start, end, ui::colors::rgba_to_hex(style.grid));
    }
  }

  // void node_editor::draw_node_body_and_outline(natural_t node_id) {
  // const node_layout& layout = layouts.at(node_id);
  // const canvas_rect r = node_screen_rect(layout);
  // const float rounding = style.node_rounding * view_transform.zoom;

  // const bool selected = is_node_selected(node_id);
  // const bool hovered = hovered_node_id == node_id;
  // const glm::vec4 outline = selected ? style.node_outline_selected : hovered ? style.node_outline_hovered :
  //                                                                              style.node_outline;

  // set_body_channel(node_id);
  // draw_list->AddRectFilled({ r.min.x, r.min.y }, { r.max.x, r.max.y },
  //                          ui::colors::rgba_to_hex(style.node_body), rounding);
  // const float header_h = header_height() * view_transform.zoom;
  // draw_list->AddRectFilled({ r.min.x, r.min.y }, { r.max.x, r.min.y + header_h },
  //                          ui::colors::rgba_to_hex(node_header_color(node_id)), rounding,
  //                          ImDrawFlags_RoundCornersTop);
  // draw_list->AddRect({ r.min.x, r.min.y }, { r.max.x, r.max.y },
  //                    ui::colors::rgba_to_hex(outline), rounding, 0,
  //                    (selected ? 2.f : 1.f) * view_transform.zoom);

  // set_content_channel(node_id);
  // for (const pin_offset& po : layout.pin_offsets) {
  //   draw_pin_dot(po, r);  ///< circle or square per pin_style.shape; filled when linked
  // }
  // }

  void node_editor::update_hover(const glm::vec2& mouse, bool canvas_hovered) {
    // hovered_node_id = hovered_pin_id = hovered_link_id = kInvalidId;
    // if (!canvas_hovered) {
    //   return;
    // }

    // for (const pin_record& rec : pins) {
    //   const float r = glm::max(style.pin_radius * view_transform.zoom * 1.75f, 6.f);
    //   if (glm::distance(mouse, rec.dot_screen_position) <= r) {
    //     hovered_pin_id = rec.id;
    //     hovered_node_id = rec.node_id;
    //     break;
    //   }
    // }

    // for (auto itr = submission_order.rbegin(); itr != submission_order.rend(); ++itr) {
    //   const node_layout& layout = layouts.at(*itr);
    //   const glm::vec2 rect_min = view_transform.to_screen(layout.position);
    //   const glm::vec2 rect_max = rect_min + layout.size * view_transform.zoom;
    //   if (mouse.x >= rect_min.x && mouse.y >= rect_min.y && mouse.x <= rect_max.x && mouse.y <= rect_max.y) {
    //     hovered_node_id = *itr;
    //   }
    // }

    // for (const link_record& rec : links) {
    //   const float tolerance = glm::max(6.f, rec.style.thickness * view_transform.zoom * 2.f);
    //   float best = std::numeric_limits<float>::max();

    //   const float d = distance_to_link(rec, mouse, tolerance);
    //   if (d < tolerance && d < best) {
    //     best = d;
    //     hovered_link_id = rec.id;
    //   }
    // }
  }

  void node_editor::process_interactions() {
    const ImGuiIO& io = ImGui::GetIO();
    const glm::vec2 mouse = { io.MousePos.x, io.MousePos.y };
    const bool canvas_hovered = ImGui::IsWindowHovered();
    // update_hover(mouse, canvas_hovered);

    if (canvas_hovered && io.MouseWheel != 0.f) {
      const float target = glm::clamp(view_transform.zoom * (io.MouseWheel > 0.f ? 1.1f : 1.f / 1.1f), style.min_zoom, style.max_zoom);
      view_transform.zoom_around(mouse, target);
    }

    switch (action) {
      case canvas_action::NONE: {
        if (!canvas_hovered || ImGui::IsAnyItemActive()) {
          break;  ///< a widget inside a node owns the mouse — canvas stays idle
        }
        // if (hovered_pin_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        //   action = canvas_action::LINKING;
        //   action_pin = hovered_pin_id;
        // } else if (hovered_node_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        //   if (!contains_id(selection_nodes, hovered_node_id)) {
        //     if (!io.KeyCtrl) {
        //       selection_nodes.clear();
        //       selection_links.clear();
        //     }
        //     selection_nodes.push_back(hovered_node_id);
        //   }
        //   pending_moves.clear();
        //   for (natural_t id : selection_nodes) {
        //     pending_moves.push_back(node_move{ id, layouts.at(id).position, {} });
        //   }
        //   action = canvas_action::DRAGGING_NODES;
        // } else if (hovered_link_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        //   if (!io.KeyCtrl) {
        //     selection_nodes.clear();
        //     selection_links.clear();
        //   }
        //   if (!contains_id(selection_links, hovered_link_id)) {
        //     selection_links.push_back(hovered_link_id);
        //   }
        // } else
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          action = canvas_action::PANNING;
          if (canvas_hovered && ImGui::GetIO().MouseWheel != 0.f) {
            view_transform.zoom *= ImGui::GetIO().MouseWheel > 0.f ? 1.1f : 1.f / 1.1f;
          }
        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          // selection_nodes.clear();
          // selection_links.clear();
        }
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) &&
            ImGui::IsKeyPressed(ImGuiKey_Delete)) {
          // delete_links.insert(delete_links.end(), selection_links.begin(), selection_links.end());
        }
      } break;

      case canvas_action::PANNING: {
        view_transform.pan -= glm::vec2{ io.MouseDelta.x, io.MouseDelta.y } / view_transform.zoom;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          action = canvas_action::NONE;
        }
      } break;

      case canvas_action::DRAGGING_NODES: {
        // const glm::vec2 delta = glm::vec2{ io.MouseDelta.x, io.MouseDelta.y } / view_transform.zoom;
        // for (natural_t id : selection_nodes) {
        //   layouts.at(id).position += delta;
        // }
        // if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        //   for (node_move& mv : pending_moves) {
        //     mv.new_position = layouts.at(mv.node_id).position;
        //     if (glm::distance(mv.old_position, mv.new_position) > 0.5f) {
        //       moves_completed.push_back(mv);
        //     }
        //   }
        //   pending_moves.clear();
        //   action = canvas_action::NONE;
        // }
      } break;

      case canvas_action::LINKING: break;
      case canvas_action::RELINKING: break;
      case canvas_action::BOX_SELECTING: break;
      default:
        OTHER_ASSERT(false, "unhandled node_editor canvas_action {}", action);
    }
  }

}  // namespace other