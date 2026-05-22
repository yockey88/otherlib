/**
 * \file status_window.hpp
 **/
#include "renderer/ui/ui_window.hpp"

#include "driver/driver.hpp"
#include "plugin/plugin.hpp"

class OTHER_CLASS status_window : public other::ui_window {
 public:
  status_window(other::event_system* event_sys)
      : ui_window(event_sys, "Status") {}
  ~status_window() override = default;

  void on_render_body() override {
  }
};

OTHER_PROVIDES(status_window, other::ui_window, "status_window")
OTHER_PLUGIN("Status Window", "0.0.1", "N/A", "Provides nice runtime info while debugging")