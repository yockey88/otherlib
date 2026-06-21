/**
 * \file ui/viewport.hpp
 **/
#ifndef OTHERLIB_UI_VIEWPORT_HPP
#define OTHERLIB_UI_VIEWPORT_HPP

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/renderer.hpp"
#include "renderer/ui/ui_window.hpp"

namespace other {

  class driver;

  namespace ui {

    class viewport : public ui_window {
     public:
      viewport(event_system& events, scope<renderer>& renderer_ptr, driver* driver_ptr);
      ~viewport() override = default;

      inline void set_display_texture(resource_handle resource) {
        display_texture_id = resource;
      }

      void on_render_header() override;
      void on_render_body() override;
      void on_pre_render_nodes() override;

     private:
      driver* driver_ptr = nullptr;
      resource_handle display_texture_id = {};
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_VIEWPORT_HPP