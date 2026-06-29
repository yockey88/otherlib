/**
 * \file ui/render-pipeline-ui/render_pipeline_viewer.hpp
 **/
#ifndef OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP
#define OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP

#include "event/event_system.hpp"

#include "renderer/pipeline_definition.hpp"
#include "renderer/renderer.hpp"
#include "renderer/util/pipeline_asset_validation.hpp"

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
      bool dirty() const { return is_dirty; }
      void mark_dirty() {
        is_dirty = true;
        // revalidate();
      }
      const detail::validation_result& validation() const { return last_validation; }

     protected:
      void on_render_header() override;
      void on_render_body() override;

     private:
      struct list_entry {
        std::string name;
        bool live;
        bool on_disk;
        filepath path;
      };
      editor_context& editor_ctx;

      std::vector<list_entry> entries;
      int32_t selected = -1;
      int32_t pending_select = -1;

      pipeline_definition working_def;
      bool has_doc = false;
      bool is_dirty = false;
      detail::validation_result last_validation;

      inline list_entry* find_entry(std::string_view name) {
        auto itr = std::find_if(entries.begin(), entries.end(), [name](const list_entry& e) { return e.name == name; });
        return itr != entries.end() ? &(*itr) : nullptr;
      }

      void draw_list();
      void draw_delete_confirm_popup();
      void draw_validation_banner();
      void draw_properties();
      void draw_required_tags();
      void draw_chain_section();
      void draw_add_chain_link_popup();

      void rebuild_list();
      int index_of(std::string_view name) const;
      void load_into_working(std::string_view pipeline_name);
      void draw_discard_confirm_popup();

      void create_new();
      void clone_current();
      void delete_current();
      void save_current();
      std::string make_unique_name(std::string_view base) const;
      filepath default_pipeline_dir() const;

      bool texture_name_combo(const char* label, std::string& out);
      bool pass_name_combo_opt(const char* label, opt<std::string>& out);

      bool chip(std::string_view text, bool closable);
      std::string tag_display_string(resource_tag t) const;
      bool tag_picker_popup(const char* id, std::string& out);
      void register_tag_string(resource_tag t, std::string_view s);

      void revalidate() {
        // last_validation = validate_pipeline_definition(working_def);
      }
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_RENDER_PIPELINE_UI_RENDER_PIPELINE_VIEWER_HPP