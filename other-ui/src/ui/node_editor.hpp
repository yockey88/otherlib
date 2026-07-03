/**
 * \file ui/node_editor.hpp
 **/
#ifndef OTHER_UI_NODE_EDITOR_HPP
#define OTHER_UI_NODE_EDITOR_HPP

#include <glm/glm.hpp>

#include "core/fnv.hpp"

#include "theme/node_editor_style.hpp"
#include "ui/node_editor_math.hpp"

namespace other {

  enum class pin_type {
    INPUT,
    OUTPUT
  };

  struct link_creation_data {
    natural_t from_pin_id;
    natural_t to_pin_id;
  };

  struct pin_drop_data {
    natural_t from_pin_id;
    glm::vec2 drop_position;
  };

  struct node_move {
    natural_t node_id;
    glm::vec2 old_position;
    glm::vec2 new_position;
  };

  struct node_canvas_config {
    int32_t link_mouse_button = 0;  //< ImGuiMouseButton_Left
    int32_t pan_mouse_button = 2;   //< ImGuiMouseButton_Middle

    float min_zoom = 0.25f;
    float max_zoom = 2.5f;
    float grid_step = 16.f;

    bool one_link_per_input = true;
    bool allow_self_links = false;
    bool detach_links_from_inputs = true;
    bool snap_to_grid = false;
    bool draw_minimap = false;

    std::function<bool(natural_t from_pin, natural_t to_pin)> can_link = nullptr;
  };

  class node_editor {
   public:
    struct pin_offset {
      natural_t pin_id = kInvalidId;
      pin_type direction = pin_type::INPUT;
      float row_center_offset_y = 0.f;
    };
    struct node_layout {
      glm::vec2 position = { 0.f, 0.f };
      glm::vec2 size = { 0.f, 0.f };
      natural_t last_submit_frame = 0;
      std::vector<pin_offset> pin_offsets;
    };
    node_editor() = default;
    ~node_editor() = default;

    constexpr static natural_t kInvalidId = static_cast<natural_t>(-1);

    static constexpr natural_t node_id(const std::string_view title) { return FNV(title); }
    static constexpr natural_t pin_id(natural_t node_id, const std::string_view pin_name, pin_type direction) {
      constexpr natural_t kFnvPrime = 0x100000001B3;
      const natural_t base = (node_id ^ FNV(pin_name)) * kFnvPrime;
      return (base & ~natural_t{ 1 }) | static_cast<natural_t>(direction);  //< direction in bit 0
    }
    static constexpr natural_t link_id(natural_t from_pin, natural_t to_pin) {
      constexpr natural_t kFnvPrime = 0x100000001B3;
      return (from_pin ^ (to_pin * kFnvPrime)) | natural_t{ 1 } << 63;  //< avoid collision of small host ids
    }

    inline glm::vec2 spawn_position() const {
      const natural_t n = layouts.size();
      return view_transform.pan + glm::vec2{
        40.f + 30.f * static_cast<float>(n % 5),
        40.f + 30.f * static_cast<float>(n % 7),
      };
    }

    void begin(const std::string_view title, const glm::vec2& size = glm::vec2(0.f, 0.f));
    void end();

    natural_t begin_node(const std::string_view title, const glm::vec4& header_color = ui::colors::kNodeHeaderColor);
    void end_node();

    inline canvas_rect node_screen_rect(const node_layout& layout) const {
      return {
        .min = view_transform.to_screen(layout.position),
        .max = view_transform.to_screen(layout.position + layout.size),
      };
    }
    inline canvas_rect canvas_screen_rect() const {
      return {
        .min = view_transform.to_screen(glm::vec2{ 0.f, 0.f }),
        .max = view_transform.to_screen(glm::vec2{ config.grid_step, config.grid_step }) * 100.f,
      };
    }

    node_canvas_config config = {};
    node_canvas_style style = {};

   private:
    struct pin_record {
      natural_t id;
      natural_t node_id;
      float row_min_y, row_max_y;

      glm::vec2 dot_screen_position;
      pin_type direction;
      pin_style style;

      bool linked = false;
    };
    struct link_record {
      natural_t id;
      natural_t from_pin, to_pin;
      link_style style;
    };

    enum class canvas_action : uint8_t {
      NONE,
      PANNING,
      DRAGGING_NODES,
      LINKING,
      RELINKING,
      BOX_SELECTING,
    };

    natural_t frame_index = 0;
    bool frame_open = false;
    bool current_node_culled = false;

    natural_t current_node = kInvalidId;
    natural_t current_pin = kInvalidId;

    natural_t hovered_node_id = kInvalidId;
    natural_t hovered_pin_id = kInvalidId;
    natural_t hovered_link_id = kInvalidId;

    canvas_transform view_transform = {};
    canvas_action action = canvas_action::NONE;

    std::vector<pin_record> pins;
    std::vector<link_record> links;

    std::unordered_map<natural_t, node_layout> layouts;
    std::unordered_map<natural_t, natural_t> submitted_nodes;

    ImDrawList* draw_list = nullptr;
    // 0 = grid + links, 1 = nodes
    ImDrawListSplitter* splitter = nullptr;

    inline bool contains_id(const std::vector<natural_t>& ids, natural_t id) {
      return std::ranges::find(ids, id) != ids.end();
    }

    float distance_to_link(const link_record& rec, const glm::vec2& point, float max_distance) const;

    void draw_grid();

    void update_hover(const glm::vec2& mouse_pos, bool canvas_hovered);
    void process_interactions();
  };

}  // namespace other

#endif  // OTHER_UI_NODE_EDITOR_HPP