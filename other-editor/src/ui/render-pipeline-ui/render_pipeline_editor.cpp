/**
 * \file ui/render-pipeline-ui/render_pipeline_editor.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_editor.hpp"

namespace other {
  namespace ui {

    render_pipeline_editor::render_pipeline_editor(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Editor"), editor_ctx(ctx) {
    }

    void render_pipeline_editor::on_render_header() {
    }

    void render_pipeline_editor::on_render_body() {
    }

  }  // namespace ui
}  // namespace other