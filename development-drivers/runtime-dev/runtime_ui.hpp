/**
 * \file development-drivers/runtime-dev/runtime_ui.hpp
 **/
#ifndef OTHER_RUNTIME_UI_HPP
#define OTHER_RUNTIME_UI_HPP

#include "event/event_system.hpp"

#include "renderer/ui/ui_window.hpp"

namespace other {

  class runtime_control_window : public ui_window {
   public:
    runtime_control_window(event_system& events)
        : ui_window(events, "Runtime Control") {}
    virtual ~runtime_control_window() override = default;

    void on_render_body() override {}
  };

}  // namespace other

#endif  // OTHER_RUNTIME_UI_HPP