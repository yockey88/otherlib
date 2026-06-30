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

    render_pipeline_viewer::render_pipeline_viewer(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Viewer"), editor_ctx(ctx) {
    }

    void render_pipeline_viewer::on_render_header() {
      const bool can_commit = has_doc && last_validation.valid;
      ImGui::BeginDisabled(!can_commit);
      {
        // if (ImGui::Button("Save")) {
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
      // draw_validation_banner();
      draw_properties();
    }

    void render_pipeline_viewer::draw_list() {
      static constexpr glm::vec4 kLiveDot = colors::hex_col_to_rgba(IM_COL32(90, 200, 120, 255));
      static constexpr glm::vec4 kDiskDot = colors::hex_col_to_rgba(IM_COL32(140, 140, 140, 255));

      rebuild_list();

      if (!ImGui::BeginChild("pl-list", ImVec2(0.f, kListHeight))) {
        ImGui::EndChild();
        return;
      }

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
      // draw_discard_confirm_popup();
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
      if (!ImGui::BeginPopupModal("confirm-delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
      }

      const list_entry* e = (selected >= 0) ? &entries[selected] : nullptr;
      ImGui::TextWrapped("Delete pipeline '%s'?", e ? e->name.c_str() : working_def.name.c_str());
      if (e && e->on_disk) {
        ImGui::TextDisabled("Removes %s from disk.", e->path.string().c_str());
      }
      if (e && e->live) {
        ImGui::TextDisabled("The running pipeline keeps its current state.");
      }
      ImGui::Separator();

      {
        scoped_color danger{ ImGuiCol_Button, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
        if (ImGui::Button("Delete")) {
          delete_current();
          ImGui::CloseCurrentPopup();
        }
      }
      ImGui::SameLine();

      if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    void render_pipeline_viewer::draw_validation_banner() {
      static const glm::vec4 kBannerOK = colors::kBalancedGreen;
      static const glm::vec4 kBannerErr = colors::kFriendlyErrorRed;

      const detail::validation_result& v = last_validation;

      glm::vec4 col = v.valid ? kBannerOK : kBannerErr;

      scoped_color bg{ ImGuiCol_ChildBg, colors::rgba_to_imvec4(glm::vec4(glm::vec3(col), 0.15f)) };
      ImGui::BeginChild("validation", ImVec2(0.f, 0.f), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
      if (v.valid) {
        scoped_color txt{ ImGuiCol_Text, colors::rgba_to_imvec4(col) };
        ImGui::Text("%s  Valid pipeline definition", unicode::kCheckMark);
      } else {
        size_t errors = v.errors.size();

        {
          scoped_color txt{ ImGuiCol_Text, colors::rgba_to_imvec4(col) };
          ImGui::Text("%s  %zu error(s)", unicode::kCrossMark, errors);
        }

        if (ImGui::TreeNodeEx("details", ImGuiTreeNodeFlags_SpanAvailWidth)) {
          for (const std::string& err : v.errors) {
            ImGui::BulletText("%s", err.c_str());
          }
          ImGui::TreePop();
        }
      }
      ImGui::EndChild();
    }

    void render_pipeline_viewer::draw_properties() {
      constexpr int kLocalBufLen = 128;
      if (inspector::begin_pipeline_properties("Properties", FNV("pl.props"))) {
        char name_buf[kLocalBufLen];
        std::ranges::fill(name_buf, '\0');
        std::ranges::copy(working_def.name, name_buf);
        if (inspector::property_text("Name", name_buf, sizeof name_buf)) {
          working_def.name = name_buf;
          mark_dirty();
        }

        int32_t ver = static_cast<int32_t>(working_def.version);
        if (inspector::property_int32("Version", ver)) {
          working_def.version = static_cast<uint32_t>(std::max(1, ver));
          mark_dirty();
        }

        if (texture_name_combo("Display", working_def.display_texture_name)) {
          mark_dirty();
        }
        inspector::end_pipeline_properties();
      }

      // draw_required_tags();
      // draw_chain_section();
    }

    void render_pipeline_viewer::draw_required_tags() {
      if (!inspector::begin_pipeline_properties("Required tags", FNV("pl.tags"))) {
        return;
      }

      // chips, wrapping across the row; iterate without ++ when we erase
      for (size_t i = 0; i < working_def.required_tags.size();) {
        if (i > 0) ImGui::SameLine();
        if (chip(tag_display_string(working_def.required_tags[i]), /*closable=*/true)) {
          working_def.required_tags.erase(working_def.required_tags.begin() + i);
          mark_dirty();
        } else {
          ++i;
        }
      }

      ImGui::SameLine();
      if (ImGui::SmallButton("+")) {
        ImGui::OpenPopup("add-tag");
      }

      std::string picked;
      if (tag_picker_popup("add-tag", picked)) {
        const resource_tag t = resource_tag::from(picked);
        const bool already = std::ranges::find(working_def.required_tags, t) != working_def.required_tags.end();
        if (!already) {
          working_def.required_tags.push_back(t);
          register_tag_string(t, picked);
          mark_dirty();
        }
      }
      inspector::end_pipeline_properties();
    }

    void render_pipeline_viewer::draw_chain_section() {
    }

    void render_pipeline_viewer::draw_add_chain_link_popup() {
    }

    bool render_pipeline_viewer::chip(const std::string_view text, bool closable) {
      scoped_id id{ text };
      scoped_style rounding{ ImGuiStyleVar_FrameRounding, 10.f };
      ImGui::BeginGroup();
      ImGui::SmallButton(std::string{ text }.c_str());
      bool closed = false;
      if (closable) {
        ImGui::SameLine(0.f, 2.f);
        if (ImGui::SmallButton(unicode::kCrossMark)) {
          closed = true;
        }
      }
      ImGui::EndGroup();
      return closed;
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

    void render_pipeline_viewer::load_into_working(const std::string_view pipeline_name) {
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
      working_def.name = make_unique_name("new-pipeline");
      has_doc = true;
      is_dirty = true;
      selected = -1;
      // sel_pass.reset();
      // sel_resource.reset();
      revalidate();
    }

    void render_pipeline_viewer::clone_current() {
      OTHER_ASSERT(has_doc, "clone with no working document");
      pipeline_definition copy = working_def;
      copy.name = make_unique_name(working_def.name + "-copy");
      working_def = std::move(copy);
      is_dirty = true;
      selected = -1;
      revalidate();
    }

    void render_pipeline_viewer::delete_current() {
      if (selected < 0) {
        has_doc = false;
        return;
      }
      const list_entry e = entries[selected];

      if (e.on_disk) {
        std::error_code ec;
        std::filesystem::remove(e.path, ec);
        if (ec) {
          CORE_LOG_ERROR("could not delete {}: {}", e.path.string(), ec.message());
        }
        // editor_ctx.driver->get_kernel().get_core_system<asset_system>().unload_asset(e.path);
      }

      has_doc = false;
      is_dirty = false;
      selected = -1;
      // sel_pass.reset();
      // sel_resource.reset();
      rebuild_list();
    }

    void render_pipeline_viewer::save_current() {
      OTHER_ASSERT(has_doc, "save with no document");
      OTHER_ASSERT(last_validation.valid, "save reached with invalid def — button should be disabled");

      const list_entry* e = (selected >= 0) ? &entries[selected] : nullptr;
      const filepath dst = (e && e->on_disk) ? e->path : default_pipeline_dir() / (working_def.name + ".toml");

      // if (write_pipeline_definition_to_file(working_def, dst)) {
      //   is_dirty = false;
      //   rebuild_list();
      //   selected = index_of(working_def.name);
      // } else {
      //   CORE_LOG_ERROR("failed to write pipeline '{}' to {}", working_def.name, dst.string());
      // }
    }

    std::string render_pipeline_viewer::make_unique_name(const std::string_view base) const {
      auto taken = [&](const std::string& n) {
        return std::ranges::any_of(entries, [&](const list_entry& e) { return e.name == n; });
      };

      std::string candidate{ base };
      for (int suffix = 2; taken(candidate); ++suffix) {
        candidate = std::string{ base } + "-" + std::to_string(suffix);
      }

      return candidate;
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

    std::string render_pipeline_viewer::tag_display_string(resource_tag t) const {
      // if (auto it = tag_strings.find(t); it != tag_strings.end()) {
      //   return it->second;
      // }

      // return std::format("#{:08x}", static_cast<natural_t>(t));
      return "";
    }

    bool render_pipeline_viewer::tag_picker_popup(const char* id, std::string& out) {
      bool committed = false;
      if (ImGui::BeginPopup(id)) {
        for (std::string_view known : kKnownTags) {
          if (ImGui::Selectable(known.data())) {
            out = known;
            committed = true;
            ImGui::CloseCurrentPopup();
          }
        }
        ImGui::Separator();

        if (ImGui::InputTextWithHint("##custom", "custom tag…", custom_tag_buf, sizeof custom_tag_buf, ImGuiInputTextFlags_EnterReturnsTrue)) {
          out = custom_tag_buf;
          custom_tag_buf[0] = '\0';
          committed = true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
      return committed;
    }

    void render_pipeline_viewer::register_tag_string(resource_tag t, const std::string_view s) {}

  }  // namespace ui
}  // namespace other