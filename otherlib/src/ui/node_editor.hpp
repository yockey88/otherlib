/**
 * \file ui/node_editor.hpp
 **/
#ifndef OTHERLIB_UI_NODE_EDITOR_HPP
#define OTHERLIB_UI_NODE_EDITOR_HPP

#include <string>

#include <glm/glm.hpp>

#include "glm/fwd.hpp"

namespace other {

  struct node_editor {
    static constexpr float kResizeHandleSize = 20.0f;
    static constexpr float kGripPadding = 4.0f;

    struct node {
      static constexpr size_t kInvalidID = static_cast<size_t>(-1);
      static constexpr float kMinNodeDimension = 50.f;

      size_t id = 0;
      std::string name = "Node";

      struct {
        bool hovered = false;
        bool selected = false;
        bool stretching = false;
        bool dragging = false;

        bool error = false;
      } state;

      struct node_render_data {
        glm::vec2 full_node_size;
        glm::vec2 global_node_pos;
        glm::vec2 full_node_max;

        glm::vec2 node_header_end;
        glm::vec2 node_body_begin;

        glm::vec2 resize_triangle_p1;
        glm::vec2 resize_triangle_p2;
        glm::vec2 resize_triangle_p3;

        void update_base_position(const glm::vec2& node_base_position, const glm::vec2& position, const glm::vec2& size);
      } render_data;

      glm::vec3 bg_color = glm::vec3(0.1f, 0.1f, 0.1f);

      glm::vec2 size = glm::vec2(100.f, 100.f);
      glm::vec2 position = glm::vec2(0.f, 0.f);

      void draw_node(const glm::vec2& node_base_position);

     private:
      void begin_node();
    };

    node_editor() = default;
    virtual ~node_editor() = default;

    inline void add_node(const node& n) {
      nodes.push_back(n);
    }

    std::vector<node> nodes;
  };

}  // namespace other

#endif  // OTHERLIB_UI_NODE_EDITOR_HPP