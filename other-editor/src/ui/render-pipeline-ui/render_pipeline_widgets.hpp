/**
 * \file ui/render-pipeline-ui/render_pipeline_widgets.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP

namespace other {
  namespace ui {
    namespace inspector {

      constexpr inline float kRenderPipelineListUiRounding = 10.f;

      bool begin_pipeline_properties(const std::string_view title, natural_t id);
      void end_pipeline_properties();

    }  // namespace inspector
  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP