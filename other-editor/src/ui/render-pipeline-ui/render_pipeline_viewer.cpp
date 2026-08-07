/**
 * \file ui/render-pipeline-ui/render_pipeline_viewer.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_viewer.hpp"

#include <algorithm>

#include "core/profiler.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"

#include "editor_driver.hpp"
#include "imgui.h"

namespace other {
  namespace ui {

    render_pipeline_viewer::render_pipeline_viewer(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Viewer"), editor_ctx(ctx) {
    }

    void render_pipeline_viewer::on_render_header() {
    }

    void render_pipeline_viewer::on_render_body() {
      PROFILE_SECTION("render_pipeline_viewer::on_render_body");
      draw_list();
      if (!has_doc) {
        ImGui::TextDisabled("Select or create a pipeline.");
        return;
      }
      draw_properties();

      if (!working_def.display_texture_name.empty()) {
        std::string pl_name = working_def.name;
        std::string tex_name = working_def.display_texture_name;
        ImTextureID text_id = editor_ctx.get_pipeline_texture_id(pl_name, tex_name);
        if (text_id != 0) {
          ImVec2 size = editor_ctx.get_pipeline_texture_size(pl_name, tex_name);
          ImVec2 avail = ImGui::GetContentRegionAvail();
          if (size.x > avail.x) {
            size.y *= avail.x / size.x;
            size.x = avail.x;
          }
          if (size.y > avail.y) {
            size.x *= avail.y / size.y;
            size.y = avail.y;
          }

          ImGui::Text("Display Texture:");
          ImGui::Image(text_id, size, ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
        } else {
          ImGui::TextDisabled("Display texture not found: %s", working_def.display_texture_name.c_str());
        }
      }
    }

    void render_pipeline_viewer::draw_list() {
      PROFILE_SECTION("render_pipeline_viewer::draw_list");
      entries = inspector::rebuild_render_pipeline_list(editor_ctx.driver->get_renderer(), editor_ctx.driver->get_kernel().get_core_system<asset_system>());
      uint32_t new_idx = inspector::draw_render_pipeline_list(entries, selected, pending_select);
      const bool current = new_idx == selected;
      if (current) {
        clear_working();
      } else if (new_idx >= 0 && new_idx < entries.size()) {
        selected = new_idx;
        load_into_working(entries[selected].name);
      }
    }

    void render_pipeline_viewer::draw_properties() {
      PROFILE_SECTION("render_pipeline_viewer::draw_properties");
      if (selected < 0 || selected >= entries.size()) {
        ImGui::TextDisabled("No pipeline selected.");
        return;
      }

      constexpr int kLocalBufLen = 128;
      if (inspector::begin_pipeline_properties("Properties", FNV("pl.props"))) {
        char name_buf[kLocalBufLen];
        std::ranges::fill(name_buf, '\0');
        inspector::display_text_field("Name", working_def.name);

        int32_t ver = static_cast<int32_t>(working_def.version);
        inspector::display_text_field("Version", std::format("{}", ver));

        inspector::end_pipeline_properties();
      }
    }

    void render_pipeline_viewer::load_into_working(const std::string_view pipeline_name) {
      PROFILE_SECTION("render_pipeline_viewer::load_into_working");
      const inspector::render_pipeline_data* e = find_entry(pipeline_name);
      OTHER_ASSERT(e != nullptr, "load_into_working: '{}.{}' not in list", (int)pipeline_name.size(), pipeline_name.data());

      if (e->live) {
        auto* pl = get_driver().get_renderer().get_pipeline(std::string{ pipeline_name });
        OTHER_ASSERT(pl != nullptr, "live pipeline '{}' disappeared", pipeline_name);
        working_def = pl->get_definition();
      }
      // only want to do this if we don't have another option as this is a little costly
      else if (e->on_disk) {
        // e->path is the virtual path, we need to look it up in the asset system to get the real path
        auto& assets = editor_ctx.driver->get_kernel().get_core_system<asset_system>();
        const asset* a = assets.get_asset_by_virtual_path(e->path);
        OTHER_ASSERT(a != nullptr, "asset for pipeline '{}' disappeared", e->path.string());
        working_def = read_pipeline_definition_from_file(a->load_path);
      } else if (auto* pl = get_driver().get_renderer().get_pipeline(std::string{ pipeline_name })) {
        working_def = pl->get_definition();
      } else {
        CORE_LOG_WARN("pipeline '{}' vanished between list build and load", pipeline_name);
        has_doc = false;
        return;
      }

      has_doc = true;
    }

    void render_pipeline_viewer::clear_working() {
      working_def = pipeline_definition{};
      has_doc = false;
      selected = -1;
    }

  }  // namespace ui
}  // namespace other