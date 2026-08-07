/**
 * \file ui/render-pipeline-ui/render_pipeline_editor.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP

#include "ui/node-editor/node_editor_display.hpp"
#include "data-structures/std_container.hpp"
#include "ui/node_editor.hpp"
#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"
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
      node_editor working_editor;

      ostd::vector<inspector::render_pipeline_data> entries;
      int32_t selected = -1;
      int32_t pending_select = -1;

      pipeline_definition working_def;
      bool has_doc = false;
      bool is_dirty = false;
      detail::validation_result last_validation;

      inline inspector::render_pipeline_data* find_entry(std::string_view name) {
        auto itr = std::find_if(entries.begin(), entries.end(), [name](const inspector::render_pipeline_data& e) { return e.name == name; });
        return itr != entries.end() ? &(*itr) : nullptr;
      }

      inline void mark_dirty() {
        is_dirty = true;
        last_validation = validate_pipeline_definition(working_def);
      }

      void draw_list();
      void draw_validation_banner();
      void draw_properties();
      void draw_node_editor();

      void draw_discard_confirm_popup();
      bool draw_texture_name_combo(const char* label, std::string& texture_name);

      void load_into_working(const std::string_view pipeline_name);
      void clear_working();

      detail::validation_result validate_pipeline_definition(const pipeline_definition& def) const;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_EDITOR_HPP