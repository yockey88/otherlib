/**
 * \file ui/render-pipeline-ui/render_pipeline_viewer.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_viewer.hpp"

#include <algorithm>

#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/render-pipeline-ui/render_pipeline_widgets.hpp"
#include "ui/ui_helpers.hpp"
#include "ui/unicode.hpp"

#include "editor_driver.hpp"

namespace other {
  namespace ui {

    static constexpr float kListHeight = 150.f;
    static constexpr float kValidationBannerHeight = 24.f;

    render_pipeline_viewer::render_pipeline_viewer(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Viewer"), editor_ctx(ctx) {
    }

    void render_pipeline_viewer::on_render_header() {
      const bool can_commit = has_doc && last_validation.valid;
      ImGui::BeginDisabled(!can_commit);
      {
        // if (ImGui::Button("Save")) {  // → disk only
        //   filepath dst = entries[selected].on_disk ?
        //     entries[selected].path :
        //     default_pipeline_dir() / (working_def.name + ".toml");
        //   if (write_pipeline_definition_to_file(working_def, dst)) {
        //     is_dirty = false;
        //     rebuild_list();
        //   }
        // }
        // ImGui::SameLine();

        // if (ImGui::Button("Apply")) {
        //   if (auto* pl = get_driver().get_renderer().get_pipeline(working_def.name)) {
        //     pl->reload(pipeline_definition(working_def));
        //   } else {
        //     get_driver().get_renderer().add_pipeline(working_def.name, working_def);
        //   }
        // }
      }
      ImGui::EndDisabled();

      if (!can_commit && has_doc) {
        // help_marker("Fix validation errors before Save/Apply.");
      }
    }

    void render_pipeline_viewer::on_render_body() {
      draw_list();
      if (!has_doc) {
        ImGui::TextDisabled("Select or create a pipeline.");
        return;
      }

      draw_validation_banner();

      constexpr static int32_t kLocalBufSize = 128;
      if (inspector::begin_pipeline_properties("Properties", FNV("pl.props"))) {
        char name_buf[kLocalBufSize];
        std::ranges::fill(name_buf, '\0');
        std::ranges::copy(working_def.name | std::views::take(kLocalBufSize - 1), name_buf);
        name_buf[kLocalBufSize - 1] = '\0';
        if (inspector::property_text("Name", name_buf, sizeof name_buf)) {
          working_def.name = name_buf;
          mark_dirty();
        }

        int32_t ver = static_cast<int32_t>(working_def.version);
        if (inspector::property_int32("Version", ver)) {
          working_def.version = ver;
          mark_dirty();
        }

        if (texture_name_combo("Display", working_def.display_texture_name)) {
          mark_dirty();
        }
        inspector::end_pipeline_properties();
      }

      draw_required_tags();
      draw_chain_section();
    }

    void render_pipeline_viewer::draw_list() {
      static constexpr glm::vec4 kLiveDot = colors::hex_col_to_rgba(IM_COL32(90, 200, 120, 255));
      static constexpr glm::vec4 kDiskDot = colors::hex_col_to_rgba(IM_COL32(140, 140, 140, 255));

      rebuild_list();
      ImGui::BeginChild("pl-list", ImVec2(0.f, kListHeight), /*border=*/true);

      for (int i = 0; i < (int)entries.size(); ++i) {
        const list_entry& e = entries[i];
        scoped_id row_id{ i };
        {
          scoped_color dot{ ImGuiCol_Text, colors::rgba_to_imvec4(e.live ? kLiveDot : kDiskDot) };
          ImGui::Text(e.live ? unicode::kFilledDot : unicode::kHollowDot);  // ● / ○
        }
        ImGui::SameLine();

        const bool is_current = (i == selected);
        std::string label = e.name;
        if (is_current && is_dirty) {
          label += " *";
        }

        if (ImGui::Selectable(label.c_str(), is_current, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
          if (is_current) {
            // re-clicking the loaded row is a no-op
          } else if (is_dirty) {
            pending_select = i;
            ImGui::OpenPopup("confirm-discard");
          } else {
            selected = i;
            load_into_working(e.name);
          }
        }

        // right-aligned source tag
        const char* tag = e.live ? "live" : "disk";
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(tag).x - 4.f);
        ImGui::TextDisabled("%s", tag);
      }

      ImGui::EndChild();
      draw_discard_confirm_popup();
    }

    void render_pipeline_viewer::draw_discard_confirm_popup() {
      if (!ImGui::BeginPopupModal("confirm-discard", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
      }

      ImGui::TextWrapped("'%s' has unsaved changes. Discard them and switch?", working_def.name.c_str());
      ImGui::Separator();
      if (ImGui::Button("Discard & switch")) {
        OTHER_ASSERT(pending_select >= 0 && pending_select < (int)entries.size(), "stale pending_select");
        selected = pending_select;
        load_into_working(entries[selected].name);
        pending_select = -1;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Keep editing")) {
        pending_select = -1;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }

    void render_pipeline_viewer::draw_delete_confirm_popup() {
    }

    void render_pipeline_viewer::draw_validation_banner() {
    }

    void render_pipeline_viewer::draw_properties() {
    }

    void render_pipeline_viewer::draw_required_tags() {
    }

    void render_pipeline_viewer::draw_chain_section() {
    }

    void render_pipeline_viewer::draw_add_chain_link_popup() {
    }

    void render_pipeline_viewer::rebuild_list() {
      entries.clear();

      for (const std::string& name : get_driver().get_renderer().get_pipeline_names()) {
        entries.push_back({
          .name = name,
          .live = true,
          .on_disk = false,
          .path = {},
        });
      }

      auto& assets = editor_ctx.driver->get_kernel().get_core_system<asset_system>();
      for (const asset* a : assets.get_assets_of_type(asset::RENDERING_PIPELINE)) {
        if (auto* e = find_entry(a->load_path.stem().string())) {
          e->on_disk = true;
          e->path = a->load_path;
        } else {
          entries.push_back({ a->load_path.stem().string(), false, true, a->load_path });
        }
      }
      std::ranges::sort(entries, [](auto& x, auto& y) { return x.name < y.name; });
    }

    int render_pipeline_viewer::index_of(std::string_view name) const {
      return -1;
    }

    void render_pipeline_viewer::load_into_working(std::string_view pipeline_name) {
      const list_entry* e = find_entry(pipeline_name);
      OTHER_ASSERT(e != nullptr, "load_into_working: '{}.{}' not in list", (int)pipeline_name.size(), pipeline_name.data());

      if (e->on_disk) {
        working_def = read_pipeline_definition_from_file(e->path);
      } else if (auto* pl = get_driver().get_renderer().get_pipeline(std::string{ pipeline_name })) {
        working_def = pl->get_definition();
      } else {
        CORE_LOG_WARN("pipeline '{}' vanished between list build and load", pipeline_name);
        has_doc = false;
        return;
      }

      has_doc = true;
      is_dirty = false;
      // sel_pass.reset();
      // sel_resource.reset();
      // for (const resource_tag& t : working_def.required_tags) {
      //   if (!tag_strings.count(t)) {
      //     tag_strings.emplace(t, tag_display_string(t));
      //   }
      // }
      revalidate();
    }

    void render_pipeline_viewer::create_new() {
      working_def = get_empty_pipeline();
      // working_def.name = make_unique_name("new-pipeline");
      // has_doc = true;
      // is_dirty = true;
      // revalidate();
    }

    void render_pipeline_viewer::clone_current() {
      OTHER_ASSERT(has_doc, "clone with no working document");
      // pipeline_definition copy = working_def;  // deep copy (all vectors copy)
      // copy.name = make_unique_name(working_def.name + "-copy");
      // working_def = std::move(copy);
      // is_dirty = true;
      // revalidate();
    }

    void render_pipeline_viewer::delete_current() {}

    void render_pipeline_viewer::save_current() {}

    std::string render_pipeline_viewer::make_unique_name(std::string_view base) const {
      return "";
    }

    filepath render_pipeline_viewer::default_pipeline_dir() const {
      return "";
    }

    bool render_pipeline_viewer::texture_name_combo(const char* label, std::string& out) {
      return false;
    }

    bool render_pipeline_viewer::pass_name_combo_opt(const char* label, opt<std::string>& out) {
      return false;
    }

    bool render_pipeline_viewer::chip(std::string_view text, bool closable) {
      return false;
    }

    std::string render_pipeline_viewer::tag_display_string(resource_tag t) const {
      return "";
    }

    bool render_pipeline_viewer::tag_picker_popup(const char* id, std::string& out) {
      return false;
    }

    void render_pipeline_viewer::register_tag_string(resource_tag t, std::string_view s) {}

  }  // namespace ui
}  // namespace other