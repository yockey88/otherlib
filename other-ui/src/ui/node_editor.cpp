/**
 * \file ui/node_editor.cpp
 **/
#include "ui/node_editor.hpp"

#include <set>

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
    node_submission_order.clear();
    pins.clear();
    pin_lookup.clear();
    links.clear();
    moves_completed.clear();

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
    OTHER_ASSERT(current_node == kInvalidId, "node_editor::end() called while inside node {}", current_node);
    OTHER_ASSERT(frame_open, "node_editor::end() called without begin");
    OTHER_ASSERT(draw_list != nullptr, "node_editor::end() ImGui::GetWindowDrawList() returned null");
    OTHER_ASSERT(splitter != nullptr, "node_editor::end() splitter is null");

    splitter->SetCurrentChannel(draw_list, 0);
    process_interactions();
    draw_nodes();
    draw_pins();
    draw_links();

    if (action == canvas_action::LINKING) {
      // const ImGuiIO& io = ImGui::GetIO();
      // draw_pending_link({ io.MousePos.x, io.MousePos.y });  ///< post-merge: on top
    }
    splitter->Merge(draw_list);

    std::erase_if(selected_nodes, [this](natural_t id) { return !submitted_nodes.contains(id); });
    // std::erase_if(selected_links, [this](natural_t id) {
    //   return std::ranges::none_of(links, [id](const link_record& l) { return l.id == id; });
    // });

    ImGui::PopStyleVar(4);
    ImGui::SetWindowFontScale(1.f);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    frame_open = false;

    draw_list = nullptr;
    splitter = nullptr;
  }

  natural_t node_editor::begin_node(const std::string_view title, const glm::vec4& header_color) {
    const natural_t nid = node_id(title);
    OTHER_ASSERT(frame_open, "begin_node('{}') called outside node_editor::begin/end", title);
    OTHER_ASSERT(current_node == kInvalidId, "begin_node('{}') called inside node {}", title, current_node);
    OTHER_ASSERT(draw_list != nullptr, "begin_node called outside of begin/end");
    OTHER_ASSERT(splitter != nullptr, "begin_node called outside of begin/end");
    OTHER_ASSERT(!submitted_nodes.contains(nid), "node '{}' submitted twice this frame — node titles must be unique per canvas", title);

    current_node = nid;
    submitted_nodes.emplace(nid, node_submission_order.size());
    node_submission_order.push_back(nid);

    const float title_w = ImGui::CalcTextSize(title.data(), title.data() + title.size()).x;
    const float node_w = title_w + 2.f * (0.75f * style.node_padding);

    auto [itr, inserted] = layouts.try_emplace(nid);
    if (inserted) {
      itr->second.title = std::string{ title };
      itr->second.position = spawn_position();
      itr->second.size = { node_w, node_w * 0.6f };
    }

    const float zoom = view_transform.zoom;
    const float pad = style.node_padding * zoom;
    const float header_h = ImGui::GetFrameHeight();

    node_layout& layout = itr->second;
    layout.last_submit_frame = frame_index;
    layout.pin_offsets.clear();
    layout.min_size = { node_w, header_h + 2.f * pad };
    layout.header_height = header_h;

    const glm::vec2 rect_min = view_transform.to_screen(layout.position);

    ImGui::PushID(title.data(), title.data() + title.size());

    node_required_width = title_w / zoom + 2.f * style.node_padding;
    node_content_bottom = rect_min.y + header_h + pad;

    ImGui::SetCursorScreenPos(ImVec2{ rect_min.x + pad, rect_min.y + header_h + pad });
    ImGui::BeginGroup();
    return nid;
  }

  void node_editor::end_node() {
    OTHER_ASSERT(current_node != kInvalidId, "end_node without begin_node");
    OTHER_ASSERT(draw_list != nullptr, "end_node called outside of begin/end");
    OTHER_ASSERT(splitter != nullptr, "end_node called outside of begin/end");
    ImGui::EndGroup();

    const float zoom = view_transform.zoom;

    node_layout& layout = layouts.at(current_node);
    const glm::vec2 rect_min = view_transform.to_screen(layout.position);

    node_content_bottom = glm::max(node_content_bottom, ImGui::GetItemRectMax().y);
    layout.min_size = { node_required_width, (node_content_bottom - rect_min.y) / zoom + style.node_padding };
    layout.size = glm::max(layout.size, layout.min_size);

    refresh_pin_dot_positions(current_node);

    ImGui::PopID();
    current_node = kInvalidId;
  }

  natural_t node_editor::begin_input_pin(const std::string_view name, const glm::vec4& color) {
    return open_pin(name, pin_type::INPUT, color);
  }

  natural_t node_editor::begin_output_pin(const std::string_view name, const glm::vec4& color) {
    return open_pin(name, pin_type::OUTPUT, color);
  }

  natural_t node_editor::open_pin(const std::string_view name, pin_type direction, const glm::vec4& color) {
    OTHER_ASSERT(current_node != kInvalidId, "pin '{}' declared outside a node", name);
    OTHER_ASSERT(current_pin == kInvalidId, "pin '{}' declared inside pin {}", name, current_pin);

    const natural_t pid = pin_id(current_node, name, direction);
    OTHER_ASSERT(!pin_lookup.contains(pid), "pin '{}' submitted twice on node {}. pin names must be unique per node", name, current_node);

    current_pin = pid;
    pin_record& rec = pins.emplace_back();
    rec.id = pid;
    rec.node_id = current_node;
    rec.direction = direction;
    rec.style.color = color;
    pin_lookup.emplace(pid, pins.size() - 1);

    const float zoom = view_transform.zoom;
    const node_layout& layout = layouts.at(current_node);
    const glm::vec2 node_min = view_transform.to_screen(layout.position);

    const float inset = (style.pin_radius + style.pin_label_gap) * zoom;
    const float row_top = ImGui::GetCursorScreenPos().y;

    float label_x = node_min.x + inset;
    if (direction == pin_type::OUTPUT) {
      const glm::vec2 label_size = { ImGui::CalcTextSize(name.data(), name.data() + name.size()).x, 0.f };
      label_x = node_min.x + layout.size.x * zoom - inset - label_size.x;
    }

    ImGui::SetCursorScreenPos(ImVec2{ label_x, row_top });
    ImGui::BeginGroup();
    ImGui::TextUnformatted(name.data(), name.data() + name.size());
    return pid;
  }

  void node_editor::end_pin() {
    OTHER_ASSERT(current_pin != kInvalidId, "end_pin without begin pin");
    ImGui::EndGroup();
    const float zoom = view_transform.zoom;
    pin_record& rec = pins[pin_lookup.at(current_pin)];
    node_layout& layout = layouts.at(rec.node_id);
    const glm::vec2 node_min = view_transform.to_screen(layout.position);

    const ImVec2 group_min = ImGui::GetItemRectMin();
    const ImVec2 group_max = ImGui::GetItemRectMax();

    const float row_h = glm::max(group_max.y - group_min.y, style.pin_spacing * zoom);

    rec.row_center_y = (group_min.y + 0.5f * row_h - node_min.y) / zoom;
    layout.pin_offsets.push_back(pin_offset{
      .pin_id = rec.id,
      .direction = rec.direction,
      .row_center_offset_y = rec.row_center_y,
    });

    const float row_w = (group_max.x - group_min.x) / zoom + style.pin_radius + style.pin_label_gap + style.node_padding;
    node_required_width = glm::max(node_required_width, row_w);
    node_content_bottom = glm::max(node_content_bottom, group_min.y + row_h);

    ImGui::SetCursorScreenPos(ImVec2{ node_min.x + style.node_padding * zoom, group_min.y + row_h + ImGui::GetStyle().ItemSpacing.y });
    current_pin = kInvalidId;
  }

  void node_editor::link(natural_t from_pin_id, natural_t to_pin_id, const glm::vec4& color) {
    natural_t lid = link_id(from_pin_id, to_pin_id);

    OTHER_ASSERT(frame_open && current_node == kInvalidId, "link({}) must be declared outside node scopes", lid);
    const auto from_itr = pin_lookup.find(from_pin_id);
    const auto to_itr = pin_lookup.find(to_pin_id);
    OTHER_ASSERT(from_itr != pin_lookup.end() && to_itr != pin_lookup.end(), "link({}) references a pin not declared this frame", lid);
    OTHER_ASSERT(pins[from_itr->second].direction == pin_type::OUTPUT && pins[to_itr->second].direction == pin_type::INPUT, "link({}) must run OUTPUT -> INPUT", lid);

    pins[from_itr->second].linked = true;
    pins[to_itr->second].linked = true;
    links.push_back(link_record{ lid, from_pin_id, to_pin_id, { .color = color } });
  }

  void node_editor::sort_nodes_for_drawing() {
    // ostd::map<natural_t, uint32_t> in_degree;
    // for (const auto& [id, layout] : layouts) {
    //   in_degree[id] = 0;
    // }
    // for (const link_record& rec : links) {
    //   const natural_t from_node = pin_lookup.at(rec.from_pin);
    //   const natural_t to_node = pin_lookup.at(rec.to_pin);
    //   if (submitted_nodes.contains(from_node) && submitted_nodes.contains(to_node)) {
    //     in_degree[to_node]++;
    //   }
    // }

    // std::set<natural_t> ready_nodes;
    // for (const auto& [nid, degree] : in_degree) {
    //   if (degree == 0) {
    //     ready_nodes.insert(nid);
    //   }
    // }

    // node_draw_order.clear();
    // while (!ready_nodes.empty()) {
    //   const natural_t nid = *ready_nodes.begin();
    //   ready_nodes.erase(ready_nodes.begin());
    //   node_draw_order.push_back(nid);

    //   for (const link_record& rec : links) {
    //     const natural_t from_node = pin_lookup.at(rec.from_pin);
    //     const natural_t to_node = pin_lookup.at(rec.to_pin);
    //     if (from_node == nid && submitted_nodes.contains(to_node)) {
    //       in_degree[to_node]--;
    //       if (in_degree[to_node] == 0) {
    //         ready_nodes.insert(to_node);
    //       }
    //     }
    //   }
    // }

    // if (node_draw_order.size() != submitted_nodes.size()) {
    //   node_draw_order = node_submission_order;
    // }
    node_draw_order = node_submission_order;
  }

  void node_editor::refresh_pin_dot_positions(natural_t node_id) {
    const node_layout& layout = layouts.at(node_id);
    for (pin_record& rec : pins) {
      if (rec.node_id != node_id) {
        continue;
      }
      const float local_x = (rec.direction == pin_type::INPUT) ? 0.f : layout.size.x;
      rec.dot_screen_position = view_transform.to_screen(layout.position + glm::vec2{ local_x, rec.row_center_y });
    }
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

  void node_editor::draw_nodes() {
    sort_nodes_for_drawing();

    ImGui::TextWrapped("Nodes sorted topologically based on linkages.");
    for (auto& n : node_draw_order) {
      auto& layout = layouts.at(n);
      ImGui::Text("node %zu: %s", n, layout.title.c_str());
    }

    // now adjust layout positions according to topo sort and linkage
    // ostd::map<natural_t, glm::vec2> new_canvas_pos;
    // for (natural_t nid : topo_sorted) {
    //   const node_layout& layout = layouts.at(nid);
    //   glm::vec2 pos = layout.position;

    //   // if we have links from already-placed nodes, place ourselves below them
    //   float max_link_y = 0.f;
    //   for (const link_record& rec : links) {
    //     if (submitted_nodes.contains(rec.from_pin) && submitted_nodes.contains(rec.to_pin)) {
    //       const natural_t from_node = pin_lookup.at(rec.from_pin);
    //       const natural_t to_node = pin_lookup.at(rec.to_pin);
    //       if (to_node == nid && new_canvas_pos.contains(from_node)) {
    //         const node_layout& from_layout = layouts.at(from_node);
    //         const float link_y = new_canvas_pos[from_node].y + from_layout.size.y;
    //         if (link_y > max_link_y) {
    //           max_link_y = link_y;
    //         }
    //       }
    //     }
    //   }
    //   if (max_link_y > pos.y) {
    //     pos.y = max_link_y + 15.f;
    //   }

    //   new_canvas_pos[nid] = pos;
    // }

    for (natural_t nid : node_draw_order) {
      node_layout& layout = layouts.at(nid);
      // layout.position = new_canvas_pos[nid];
      const glm::vec2 rect_min = view_transform.to_screen(layout.position);
      const glm::vec2 rect_max = rect_min + layout.size * view_transform.zoom;

      const float zoom = view_transform.zoom;
      const float pad = style.node_padding * zoom;

      const glm::vec4 header_color = (hovered_node_id == nid) ? style.node_header_hovered : style.node_header;

      if (layout.size != glm::vec2{ 0.f, 0.f }) {
        const float rounding = style.node_rounding * zoom;
        draw_list->AddRectFilled(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_max.y }, ui::colors::rgba_to_hex(style.node_body), rounding);
        draw_list->AddRectFilled(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_min.y + layout.header_height }, ui::colors::rgba_to_hex(header_color), rounding, ImDrawFlags_RoundCornersTop);
        draw_list->AddText(ImVec2{ rect_min.x + pad, rect_min.y + 0.5f * (layout.header_height - ImGui::GetTextLineHeight()) }, ui::colors::rgba_to_hex(style.node_title_text), layout.title.c_str(), layout.title.c_str() + layout.title.size());
      }

      const bool selected = contains_id(selected_nodes, nid) || resize_node == nid;
      const bool hovered = hovered_node_id == nid;
      glm::vec4 outline = {};
      if (selected) {
        outline = style.node_outline_selected;
      } else if (hovered) {
        outline = style.node_outline_hovered;
      } else {
        outline = style.node_outline;
      }
      draw_list->AddRect(ImVec2{ rect_min.x, rect_min.y }, ImVec2{ rect_max.x, rect_max.y }, ui::colors::rgba_to_hex(outline),
                         style.node_rounding * zoom, 0, (selected ? 2.f : 1.f) * zoom);
    }
  }

  void node_editor::draw_pins() {
    for (pin_record& rec : pins) {
      ImVec2 p = { rec.dot_screen_position.x, rec.dot_screen_position.y };
      draw_list->AddCircleFilled(p, style.pin_radius * view_transform.zoom, ui::colors::rgba_to_hex(rec.style.color));
    }
  }

  void node_editor::draw_links() {
    for (const link_record& rec : links) {
      const glm::vec2 p0 = pins[pin_lookup.at(rec.from_pin)].dot_screen_position;
      const glm::vec2 p3 = pins[pin_lookup.at(rec.to_pin)].dot_screen_position;

      const bool selected = false;  // contains_id(selection_links, rec.id);
      const bool hovered = hovered_link_id == rec.id;

      const glm::vec4 color = selected ? style.node_outline_selected : rec.style.color;
      const float thickness = rec.style.thickness * view_transform.zoom * (selected || hovered ? 1.5f : 1.f);
      const float tangent = glm::min(rec.style.tangent, 0.5f * glm::distance(p0, p3)) * view_transform.zoom;
      draw_list->AddBezierCubic(ImVec2{ p0.x, p0.y }, ImVec2{ p0.x + tangent, p0.y }, ImVec2{ p3.x - tangent, p3.y }, ImVec2{ p3.x, p3.y }, ui::colors::rgba_to_hex(color), thickness);
    }
  }

  void node_editor::update_hover(const glm::vec2& mouse, bool canvas_hovered) {
    hovered_node_id = hovered_pin_id = hovered_link_id = kInvalidId;
    if (!canvas_hovered) {
      return;
    }

    for (const pin_record& rec : pins) {
      const float r = glm::max(style.pin_radius * view_transform.zoom * 1.75f, 6.f);
      if (glm::distance(mouse, rec.dot_screen_position) <= r) {
        hovered_pin_id = rec.id;
        hovered_node_id = rec.node_id;
        return;
      }
    }

    for (auto itr = node_submission_order.rbegin(); itr != node_submission_order.rend(); ++itr) {
      const node_layout& layout = layouts.at(*itr);
      const glm::vec2 rect_min = view_transform.to_screen(layout.position);
      const glm::vec2 rect_max = rect_min + layout.size * view_transform.zoom;
      if (mouse.x >= rect_min.x && mouse.y >= rect_min.y && mouse.x <= rect_max.x && mouse.y <= rect_max.y) {
        hovered_node_id = *itr;
        break;
      }
    }

    for (const link_record& rec : links) {
      const float tolerance = glm::max(6.f, rec.style.thickness * view_transform.zoom * 2.f);
      float best = std::numeric_limits<float>::max();

      const float d = distance_to_link(rec, mouse, tolerance);
      if (d < tolerance && d < best) {
        best = d;
        hovered_link_id = rec.id;
        break;
      }
    }
  }

  void node_editor::process_interactions() {
    const ImGuiIO& io = ImGui::GetIO();
    const glm::vec2 mouse = { io.MousePos.x, io.MousePos.y };
    const bool canvas_hovered = ImGui::IsWindowHovered();
    update_hover(mouse, canvas_hovered);

    if (canvas_hovered && io.MouseWheel != 0.f) {
      const float target = glm::clamp(view_transform.zoom * (io.MouseWheel > 0.f ? 1.1f : 1.f / 1.1f), style.min_zoom, style.max_zoom);
      view_transform.zoom_around(mouse, target);
    }

    switch (action) {
      case canvas_action::NONE: {
        if (!canvas_hovered) {
          break;
        }

        bool on_grip = false;
        constexpr float kResizeGripSize = 5.f;

        if (hovered_node_id != kInvalidId && hovered_pin_id == kInvalidId) {
          const node_layout& layout = layouts.at(hovered_node_id);
          const glm::vec2 rect_max = view_transform.to_screen(layout.position) + layout.size * view_transform.zoom;
          on_grip = mouse.x >= rect_max.x - kResizeGripSize && mouse.y >= rect_max.y - kResizeGripSize;
          if (on_grip) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
          }
        }

        const bool is_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        if (on_grip && is_click) {  //< hell ya
          const node_layout& layout = layouts.at(hovered_node_id);
          resize_node = hovered_node_id;
          resize_old_size = layout.size;
          resize_grab_offset = (layout.position + layout.size) - view_transform.to_canvas(mouse);
          action = canvas_action::RESIZING_NODE;
        }
        // if (hovered_pin_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        //   action = canvas_action::LINKING;
        //   // action_pin = hovered_pin_id;
        // } else
        if (hovered_node_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          active_node_id = hovered_node_id;
          if (!contains_id(selected_nodes, hovered_node_id)) {
            selected_nodes.push_back(hovered_node_id);
          }

          for (natural_t id : selected_nodes) {
            pending_moves.push_back(node_move{ id, layouts.at(id).position, {} });
          }

          action = canvas_action::DRAGGING_NODES;
        }
        // else if (hovered_link_id != kInvalidId && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        //   if (!io.KeyCtrl) {
        //     selected_nodes.clear();
        //     // selected_links.clear();
        //   }
        //   // if (!contains_id(selected_links, hovered_link_id)) {
        //   //   selected_links.push_back(hovered_link_id);
        //   // }
        // }
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
          action = canvas_action::PANNING;
        } else if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
          selected_nodes.clear();
          // selected_links.clear();
        }
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
          // delete_links.insert(delete_links.end(), selected_links.begin(), selected_links.end());
        }
      } break;

      case canvas_action::PANNING: {
        view_transform.pan += glm::vec2{ io.MouseDelta.x, io.MouseDelta.y } / view_transform.zoom;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          action = canvas_action::NONE;
        }
      } break;

      case canvas_action::DRAGGING_NODES: {
        const glm::vec2 delta = glm::vec2{ io.MouseDelta.x, io.MouseDelta.y } / view_transform.zoom;
        for (natural_t id : selected_nodes) {
          layouts.at(id).position += delta;
          refresh_pin_dot_positions(id);
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          for (node_move& mv : pending_moves) {
            mv.new_position = layouts.at(mv.node_id).position;
            if (glm::distance(mv.old_position, mv.new_position) > 0.5f) {
              moves_completed.push_back(mv);
            }
          }
          pending_moves.clear();
          action = canvas_action::NONE;
        }
      } break;

      case canvas_action::RESIZING_NODE: {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        node_layout& layout = layouts.at(resize_node);
        layout.size = view_transform.to_canvas(mouse) + resize_grab_offset - layout.position;
        refresh_pin_dot_positions(resize_node);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          if (glm::distance(resize_old_size, layout.size) > 0.5f) {
          } else {
            layout.size = resize_old_size;
          }
          resize_node = kInvalidId;
          action = canvas_action::NONE;
        }
      } break;

      case canvas_action::LINKING:
      case canvas_action::RELINKING:
      case canvas_action::BOX_SELECTING:
        CORE_LOG_WARN("Canvas action {} not implemented yet", action);
        break;
      default:
        OTHER_ASSERT(false, "unhandled node_editor canvas_action {}", action);
    }
  }

}  // namespace other