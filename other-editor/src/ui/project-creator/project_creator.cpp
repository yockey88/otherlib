/**
 * \file ui/project-creator/project_creator.cpp
 **/
#include "ui/project-creator/project_creator.hpp"

#include "ui/inspector_widgets.hpp"
#include "ui/project-creator/project_creator_widgets.hpp"

namespace other {
  namespace ui {

    project_creator::project_creator(editor_context& ctx, event_system& events)
        : ui_window(&events, "Project Creator"), editor_ctx(ctx) {
    }

    void project_creator::on_render_header() {
    }

    void project_creator::on_render_body() {
      ImGui::BeginChild("##project-creator-body", ImVec2(0, 0), false);

      inspector::begin_property_row("Project Settings");
      inspector::input_text_field("Name", project_name_buf.data(), kProjectNameBufferSize);
      inspector::end_property_row();

      ImGui::EndChild();
    }

  }  // namespace ui
}  // namespace other