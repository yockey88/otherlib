/**
 * \file ui/render-pipeline-ui/render_pipeline_widgets.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP

#include "renderer/renderer.hpp"
#include "data-structures/std_container.hpp"

#include "driver/systems/asset_system.hpp"

namespace other {
  namespace ui {
    namespace inspector {

      constexpr inline float kRenderPipelineListUiRounding = 10.f;

      struct render_pipeline_data {
        std::string name;
        bool live;
        bool on_disk;
        filepath path;
      };

      ostd::vector<render_pipeline_data> rebuild_render_pipeline_list(const renderer& r, const asset_system& assets);
      int32_t draw_render_pipeline_list(const ostd::vector<render_pipeline_data>& entries, int32_t selected_index, int32_t pending_select_index, bool current_dirty = false);

      bool begin_pipeline_properties(const std::string_view title, natural_t id);
      void end_pipeline_properties();

    }  // namespace inspector
  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_WIDGETS_HPP