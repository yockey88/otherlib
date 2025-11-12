/**
 * \file ui/node_editor.hpp
 **/
#ifndef OTHERLIB_UI_NODE_EDITOR_HPP
#define OTHERLIB_UI_NODE_EDITOR_HPP

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "core/defines.hpp"

#include "renderer/ui/ui_window.hpp"

#include "ui/node_editor_canvas_node.hpp"

namespace other {
  namespace ui {

    struct node_editor : public ui_window {
      node_editor(event_system& events);
      virtual ~node_editor() = default;

      void add_editor_node(const std::string_view node_name, uint8_t input_pins = 0, uint8_t output_pins = 0);

      void on_pre_render_nodes() override;
      void on_post_render_nodes() override;

     private:
      friend struct node_editor_node;
      node_editor_node* selected_node = nullptr;

      natural_t canvas_id = 0;

      std::map<natural_t, natural_t> node_name_hashes;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_NODE_EDITOR_HPP