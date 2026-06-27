/**
 * \file ui/viewport.hpp
 **/
#ifndef OTHERLIB_UI_VIEWPORT_VIEWPORT_HPP
#define OTHERLIB_UI_VIEWPORT_VIEWPORT_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/renderer.hpp"

#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {

  class driver;

  namespace ui {

    class viewport : public ui_window {
     public:
      viewport(editor_context& ctx, event_system& events, renderer& renderer_ptr, driver* driver_ptr);
      ~viewport() override = default;

      inline void set_display_texture(resource_handle resource) {
        display_texture_id = resource;
      }

      void on_render_header() override;
      void on_render_body() override;
      void on_pre_render_nodes() override;

     private:
      driver* driver_ptr = nullptr;
      editor_context& editor_ctx;

      resource_handle display_texture_id = {};
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_VIEWPORT_VIEWPORT_HPP