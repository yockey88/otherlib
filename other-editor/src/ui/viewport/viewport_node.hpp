/**
 * \file ui/viewport/viewport_node.hpp
 **/
#ifndef OTHERLIB_UI_VIEWPORT_VIEWPORT_NODE_HPP
#define OTHERLIB_UI_VIEWPORT_VIEWPORT_NODE_HPP

#include "renderer/renderer.hpp"

#include "ui/ui_node.hpp"

#include "editor_context.hpp"

namespace other {

  class driver;

  namespace ui {

    struct viewport_node : public ui_node {
      viewport_node(editor_context& ctx, renderer& renderer_instance, driver* driver_ptr, ui_window* parent, const std::string_view node_title)
          : ui_node(parent, node_title), renderer_instance(renderer_instance), editor_ctx(ctx) {
      }
      virtual ~viewport_node() = default;

      renderer& renderer_instance;

      void on_prepare_render() override;
      void on_render_node_header() override;
      void on_render_node_body() override;
      void on_render_end() override;

     private:
      editor_context& editor_ctx;

      ImVec2 header_size = ImVec2(0, 0);
      ImVec2 previous_size = ImVec2(0, 0);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_VIEWPORT_VIEWPORT_NODE_HPP