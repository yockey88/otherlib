/**
 * \file ui/render-pipeline-ui/render_pipeline_viewer.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP

#include "event/event_system.hpp"

#include "renderer/pipeline_definition.hpp"
#include "renderer/renderer.hpp"
#include "renderer/util/pipeline_asset_validation.hpp"

#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"
#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {
  namespace ui {

    class render_pipeline_viewer : public ui_window {
     public:
      render_pipeline_viewer(editor_context& ctx, event_system& events);
      ~render_pipeline_viewer() override = default;

      pipeline_definition* working() { return has_doc ? &working_def : nullptr; }
      const std::string& working_name() const { return working_def.name; }

     protected:
      void on_render_header() override;
      void on_render_body() override;

     private:
      static constexpr float kValidationBannerHeight = 24.f;
      static constexpr size_t kTagBuffSize = 64;
      static constexpr std::array<std::string_view, 7> kKnownTags = {
        "camera", "model", "material", "bone", "light", "simulation_environment", "screen"
      };

      editor_context& editor_ctx;

      std::vector<inspector::render_pipeline_data> entries;
      int32_t selected = -1;
      int32_t pending_select = -1;

      pipeline_definition working_def;
      bool has_doc = false;

      inline inspector::render_pipeline_data* find_entry(std::string_view name) {
        auto itr = std::find_if(entries.begin(), entries.end(), [name](const inspector::render_pipeline_data& e) { return e.name == name; });
        return itr != entries.end() ? &(*itr) : nullptr;
      }

      void draw_list();
      void draw_properties();

      void load_into_working(const std::string_view pipeline_name);
      void clear_working();
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP