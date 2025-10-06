/**
 * \file renderer/ui/nodes/hbox_node.hpp
 **/
#ifndef OTHER_RENDERER_UI_NODES_HBOX_NODE_HPP
#define OTHER_RENDERER_UI_NODES_HBOX_NODE_HPP

#include "renderer/ui/ui_node.hpp"

namespace other {

  struct hbox_node : public ui_node {
    hbox_node(ui_window* parent, const std::string_view node_title, const glm::vec2& size_arg = { 0.f, 0.f }, int32_t child_flags = 0, int32_t window_flags = 0)
        : ui_node(parent, node_title, size_arg, child_flags, window_flags) {
    }
    virtual ~hbox_node() = default;

   protected:
    void on_render_start() override;
    void render_node() override;
    void override_child_rendering() override;
  };

}  // namespace other

#endif  // OTHER_RENDERER_UI_NODES_HBOX_NODE_HPP