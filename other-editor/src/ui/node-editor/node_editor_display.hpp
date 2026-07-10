/**
 * \file ui/node-editor/node_editor_display.hpp
 **/
#ifndef OTHERLIB_UI_NODE_EDITOR_DISPLAY_HPP
#define OTHERLIB_UI_NODE_EDITOR_DISPLAY_HPP

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/defines.hpp"

#include "ui/node-editor/node_editor_canvas.hpp"
#include "ui/ui_window.hpp"

namespace other {
  namespace ui {

    struct node_editor_display {
      using node_update_fn_t = void (*)(natural_t node_id);
      using node_display_fn_t = void (*)(natural_t node_id, ImRect body_rect);
      using node_add_link_fn_t = void (*)(natural_t node_id, natural_t pin_id_start, natural_t pin_id_end);
      using node_remove_link_fn_t = void (*)(natural_t node_id, natural_t link_id);
      struct node_display_data {
        natural_t node_id;
        natural_t name_hash;
        node_update_fn_t update_fn = nullptr;
        node_display_fn_t display_fn = nullptr;
        node_add_link_fn_t add_link_fn = nullptr;
        node_remove_link_fn_t remove_link_fn = nullptr;
      };

      node_editor_display()
          : canvas(this) {}
      virtual ~node_editor_display() = default;

      natural_t add_editor_node(const std::string_view node_name, uint8_t input_pins = 0, uint8_t output_pins = 0);
      void connect_node_pins(const std::string_view from_node, uint8_t from_pin_idx, const std::string_view to_node, uint8_t to_pin_idx);

      void clear_nodes();

      void reorganize_nodes();

      void on_prepare_render();
      void on_render_end();

      void render_node_body(natural_t node_id, ImRect body_rect);

      void set_display_fn(natural_t node_id, node_display_fn_t display_fn);
      void set_add_link_fn(natural_t node_id, node_add_link_fn_t add_link_fn);
      void set_remove_link_fn(natural_t node_id, node_remove_link_fn_t remove_link_fn);
      void set_update_fn(natural_t node_id, node_update_fn_t update_fn);

     private:
      friend struct node_editor_node;

      node_editor_canvas canvas;

      node_editor_node* selected_node = nullptr;
      ostd::map<natural_t, node_display_data> node_data;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_NODE_EDITOR_DISPLAY_HPP