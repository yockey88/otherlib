/**
 * \file project_window.hpp
 **/
#ifndef OTHER_EDITOR_SRC_PROJECT_WINDOW_HPP
#define OTHER_EDITOR_SRC_PROJECT_WINDOW_HPP

#include "renderer/ui/ui_window.hpp"

#include "imgui.h"


namespace other {
  namespace ui {

    class project_window : public ui_window {
     public:
      project_window(event_system& events)
          : ui_window(events, "Project Settings", true, ImGuiWindowFlags_MenuBar) {}
      ~project_window() override = default;

      void on_render_header() override;
      void on_render_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_SRC_PROJECT_WINDOW_HPP