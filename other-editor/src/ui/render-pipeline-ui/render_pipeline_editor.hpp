/**
 * \file ui/render-pipeline-ui/render_pipeline_editor.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP

#include "ui/node-editor/node_editor_display.hpp"
#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {
  namespace ui {

    class render_pipeline_editor : public ui_window {
     public:
      render_pipeline_editor(editor_context& ctx, event_system& events);
      virtual ~render_pipeline_editor() = default;

     protected:
      void on_render_header() override;
      void on_render_body() override;

     private:
      editor_context& editor_ctx;

      node_editor_display node_editor;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP