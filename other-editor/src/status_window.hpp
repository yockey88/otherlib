/**
 * \file status_window.hpp
 **/
#ifndef OTHER_EDITOR_STATUS_WINDOW_HPP
#define OTHER_EDITOR_STATUS_WINDOW_HPP

#include "renderer/ui/ui_window.hpp"

namespace other {

  class editor_driver;

  namespace ui {

    class status_window : public ui_window {
     public:
      status_window(event_system& event_sys, editor_driver* driver)
          : ui_window(event_sys, "Status"), driver_ptr(driver) {}
      ~status_window() override = default;

      void on_render_body() override;

     private:
      editor_driver* driver_ptr = nullptr;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_STATUS_WINDOW_HPP