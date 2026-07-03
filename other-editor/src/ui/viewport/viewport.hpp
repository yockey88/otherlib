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
      viewport(editor_context& ctx, event_system& events, renderer& renderer_instance, driver* driver_ptr);
      ~viewport() override = default;

      bool override_render() override { return true; }
      void custom_render() override;

     private:
      renderer& renderer_instance;
      driver* driver_ptr = nullptr;
      editor_context& editor_ctx;

      ImVec2 header_size = ImVec2(0, 0);
      ImVec2 previous_size = ImVec2(0, 0);

      resource_handle display_texture_id = {};

      void draw_viewport(other::viewport& vp);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_VIEWPORT_VIEWPORT_HPP