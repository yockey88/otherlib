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
#include "imgui.h"

namespace other {
  namespace ui {

    render_pipeline_viewer::render_pipeline_viewer(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Viewer"), editor_ctx(ctx) {
    }

    void render_pipeline_viewer::on_render_header() {
      const bool can_commit = has_doc && last_validation.valid;
      ImGui::BeginDisabled(!can_commit);
      {
        if (ImGui::Button("Save")) {
          // filepath dst = entries[selected].on_disk ?
          //   entries[selected].path :
          //   default_pipeline_dir() / (working_def.name + ".toml");
          // if (write_pipeline_definition_to_file(working_def, dst)) {
          //   is_dirty = false;
          //   rebuild_list();
          // }
        }
        ImGui::SameLine();

        if (ImGui::Button("Apply")) {
          // if (auto* pl = get_driver().get_renderer().get_pipeline(working_def.name)) {
          //   pl->reload(pipeline_definition(working_def));
          // } else {
          //   get_driver().get_renderer().add_pipeline(working_def.name, working_def);
          // }
        }
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
      static constexpr glm::vec4 kLiveDot = colors::hex_col_to_rgba(IM_COL32(90, 200, 120, 255));
      static constexpr glm::vec4 kDiskDot = colors::hex_col_to_rgba(IM_COL32(140, 140, 140, 255));
      static constexpr float kTitleHeaderHeight = 24.f;

      rebuild_list();

      if (!ImGui::BeginChild("pl-list", ImVec2(0.f, kListHeight))) {
        ImGui::EndChild();
        return;
      }

      /// place a dark background behind the list
      auto* draw_list = ImGui::GetWindowDrawList();
      OTHER_ASSERT(draw_list != nullptr, "ImGui::GetWindowDrawList() returned null in render_pipeline_viewer::draw_list");
      ImVec2 p0 = ImGui::GetCursorScreenPos();
      ImVec2 current_child_size = ImGui::GetContentRegionAvail();

      // list background
      {
        ImVec2 p1 = ImVec2(p0.x + current_child_size.x, p0.y + current_child_size.y);
        draw_list->AddRectFilled(p0, p1, colors::rgba_to_hex(colors::kBG1), ui::inspector::kRenderPipelineListUiRounding);
      }
      // list title header
      {
        ImVec2 p1 = ImVec2(p0.x + current_child_size.x, p0.y + kTitleHeaderHeight);
        draw_list->AddRectFilled(p0, p1, colors::rgba_to_hex(colors::kBG2), ui::inspector::kRenderPipelineListUiRounding);
        ImGui::SetCursorScreenPos(ImVec2(p0.x + 8.f, p0.y + 4.f));
        ImGui::Text("Pipelines");
      }

      for (size_t i = 0; i < entries.size(); ++i) {
        const list_entry& e = entries[i];
        scoped_id row_id{ i };
        {
          scoped_color dot{ ImGuiCol_Text, colors::rgba_to_imvec4(e.live ? kLiveDot : kDiskDot) };
          std::string mod_text_str = std::string(e.live ? unicode::kStatusDot : unicode::kHollowDot);  // ● / ○
          ImGui::Text("%s", mod_text_str.c_str());
        }
        ImGui::SameLine();

        const bool is_current = (i == selected);
        std::string label = e.name;
        if (is_current && is_dirty) {
          label += " *";
        }

        if (ImGui::Selectable(label.c_str(), is_current, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
          if (is_current) {
            clear_working();
          } else if (is_dirty) {
            pending_select = i;
            ImGui::OpenPopup("confirm-discard");
          } else {
            selected = i;
            load_into_working(e.name);
          }
        }

        // right-aligned source tag
        std::string tag = " ";
        if (e.live) {
          tag += "live ";
        } else {
          tag += "     ";
        }
        if (e.on_disk) {
          tag += "disk";
        } else {
          tag += "    ";
        }
        tag += " ";
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(tag.c_str()).x - 4.f);
        ImGui::TextDisabled("%s", tag.c_str());
      }

      ImGui::EndChild();
      draw_discard_confirm_popup();
    }

    void render_pipeline_viewer::draw_validation_banner() {
      static const glm::vec4 kBannerOK = colors::kBalancedGreen;
      static const glm::vec4 kBannerErr = colors::kFriendlyErrorRed;

      const detail::validation_result& v = last_validation;

      glm::vec4 col = v.valid ? kBannerOK : kBannerErr;

      scoped_color bg{ ImGuiCol_ChildBg, colors::rgba_to_imvec4(glm::vec4(glm::vec3(col), 0.15f)) };
      scoped_style rounding(ImGuiStyleVar_ChildRounding, ui::inspector::kRenderPipelineListUiRounding);
      ImGui::BeginChild("validation", ImVec2(0.f, 0.f), ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
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

    // void render_pipeline_viewer::draw_delete_confirm_popup() {
    //   if (!ImGui::BeginPopupModal("confirm-delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    //     return;
    //   }

    //   const list_entry* e = (selected >= 0) ? &entries[selected] : nullptr;
    //   ImGui::TextWrapped("Delete pipeline '%s'?", e ? e->name.c_str() : working_def.name.c_str());
    //   if (e && e->on_disk) {
    //     ImGui::TextDisabled("Removes %s from disk.", e->path.string().c_str());
    //   }
    //   if (e && e->live) {
    //     ImGui::TextDisabled("The running pipeline keeps its current state.");
    //   }
    //   ImGui::Separator();

    //   {
    //     scoped_color danger{ ImGuiCol_Button, colors::rgba_to_imvec4(colors::kFriendlyErrorRed) };
    //     if (ImGui::Button("Delete")) {
    //       delete_current();
    //       ImGui::CloseCurrentPopup();
    //     }
    //   }
    //   ImGui::SameLine();

    //   if (ImGui::Button("Cancel")) {
    //     ImGui::CloseCurrentPopup();
    //   }
    //   ImGui::EndPopup();
    // }

    // void render_pipeline_viewer::draw_required_tags() {
    //   if (!inspector::begin_pipeline_properties("Required tags", FNV("pl.tags"))) {
    //     return;
    //   }

    //   // chips, wrapping across the row; iterate without ++ when we erase
    //   for (size_t i = 0; i < working_def.required_tags.size();) {
    //     if (i > 0) ImGui::SameLine();
    //     if (chip(tag_display_string(working_def.required_tags[i]), /*closable=*/true)) {
    //       working_def.required_tags.erase(working_def.required_tags.begin() + i);
    //       mark_dirty();
    //     } else {
    //       ++i;
    //     }
    //   }

    //   ImGui::SameLine();
    //   if (ImGui::SmallButton("+")) {
    //     ImGui::OpenPopup("add-tag");
    //   }

    //   std::string picked;
    //   if (tag_picker_popup("add-tag", picked)) {
    //     const resource_tag t = resource_tag::from(picked);
    //     const bool already = std::ranges::find(working_def.required_tags, t) != working_def.required_tags.end();
    //     if (!already) {
    //       working_def.required_tags.push_back(t);
    //       register_tag_string(t, picked);
    //       mark_dirty();
    //     }
    //   }
    //   inspector::end_pipeline_properties();
    // }

    // void render_pipeline_viewer::draw_chain_section() {
    // }

    // void render_pipeline_viewer::draw_add_chain_link_popup() {
    // }

    // bool render_pipeline_viewer::chip(const std::string_view text, bool closable) {
    //   scoped_id id{ text };
    //   scoped_style rounding{ ImGuiStyleVar_FrameRounding, 10.f };
    //   ImGui::BeginGroup();
    //   ImGui::SmallButton(std::string{ text }.c_str());
    //   bool closed = false;
    //   if (closable) {
    //     ImGui::SameLine(0.f, 2.f);
    //     if (ImGui::SmallButton(unicode::kCrossMark)) {
    //       closed = true;
    //     }
    //   }
    //   ImGui::EndGroup();
    //   return closed;
    // }

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
        if (auto* e = find_entry(a->virtual_path.stem().string())) {
          e->on_disk = true;
          e->path = a->virtual_path;
        } else {
          entries.push_back({
            .name = a->virtual_path.stem().string(),
            .live = false,
            .on_disk = true,
            .path = a->virtual_path,
          });
        }
      }

      std::ranges::sort(entries, [](auto& x, auto& y) { return x.name < y.name; });
    }

    void render_pipeline_viewer::load_into_working(const std::string_view pipeline_name) {
      const list_entry* e = find_entry(pipeline_name);
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
      is_dirty = false;
      revalidate();
    }

    void render_pipeline_viewer::clear_working() {
      working_def = pipeline_definition{};
      has_doc = false;
      is_dirty = false;
      selected = -1;
    }

    // int render_pipeline_viewer::index_of(std::string_view name) const {
    //   return -1;
    // }

    // void render_pipeline_viewer::create_new() {
    //   working_def = get_empty_pipeline();
    //   working_def.name = make_unique_name("new-pipeline");
    //   has_doc = true;
    //   is_dirty = true;
    //   selected = -1;
    //   // sel_pass.reset();
    //   // sel_resource.reset();
    //   revalidate();
    // }

    // void render_pipeline_viewer::clone_current() {
    //   OTHER_ASSERT(has_doc, "clone with no working document");
    //   pipeline_definition copy = working_def;
    //   copy.name = make_unique_name(working_def.name + "-copy");
    //   working_def = std::move(copy);
    //   is_dirty = true;
    //   selected = -1;
    //   revalidate();
    // }

    // void render_pipeline_viewer::delete_current() {
    //   if (selected < 0) {
    //     has_doc = false;
    //     return;
    //   }
    //   const list_entry e = entries[selected];

    //   if (e.on_disk) {
    //     std::error_code ec;
    //     std::filesystem::remove(e.path, ec);
    //     if (ec) {
    //       CORE_LOG_ERROR("could not delete {}: {}", e.path.string(), ec.message());
    //     }
    //     // editor_ctx.driver->get_kernel().get_core_system<asset_system>().unload_asset(e.path);
    //   }

    //   has_doc = false;
    //   is_dirty = false;
    //   selected = -1;
    //   // sel_pass.reset();
    //   // sel_resource.reset();
    //   rebuild_list();
    // }

    // void render_pipeline_viewer::save_current() {
    //   OTHER_ASSERT(has_doc, "save with no document");
    //   OTHER_ASSERT(last_validation.valid, "save reached with invalid def — button should be disabled");

    //   const list_entry* e = (selected >= 0) ? &entries[selected] : nullptr;
    //   const filepath dst = (e && e->on_disk) ? e->path : default_pipeline_dir() / (working_def.name + ".toml");

    //   // if (write_pipeline_definition_to_file(working_def, dst)) {
    //   //   is_dirty = false;
    //   //   rebuild_list();
    //   //   selected = index_of(working_def.name);
    //   // } else {
    //   //   CORE_LOG_ERROR("failed to write pipeline '{}' to {}", working_def.name, dst.string());
    //   // }
    // }

    // std::string render_pipeline_viewer::make_unique_name(const std::string_view base) const {
    //   auto taken = [&](const std::string& n) {
    //     return std::ranges::any_of(entries, [&](const list_entry& e) { return e.name == n; });
    //   };

    //   std::string candidate{ base };
    //   for (int suffix = 2; taken(candidate); ++suffix) {
    //     candidate = std::string{ base } + "-" + std::to_string(suffix);
    //   }

    //   return candidate;
    // }

    // filepath render_pipeline_viewer::default_pipeline_dir() const {
    //   return "";
    // }

    bool render_pipeline_viewer::texture_name_combo(const char* label, std::string& out) {
      bool changed = false;
      inspector::begin_property_row(label);
      if (ImGui::BeginCombo("##tex", out.empty() ? "<none>" : out.c_str())) {
        if (ImGui::Selectable("<none>", out.empty())) {
          out.clear();
          changed = true;
        }
        for (const pipeline_texture_definition& t : working_def.textures) {
          if (ImGui::Selectable(t.name.c_str(), t.name == out)) {
            out = t.name;
            changed = true;
          }
        }
        ImGui::EndCombo();
      }
      inspector::end_property_row();
      return changed;
    }

    // bool render_pipeline_viewer::pass_name_combo_opt(const char* label, opt<std::string>& out) {
    //   return false;
    // }

    // std::string render_pipeline_viewer::tag_display_string(resource_tag t) const {
    //   // if (auto it = tag_strings.find(t); it != tag_strings.end()) {
    //   //   return it->second;
    //   // }

    //   // return std::format("#{:08x}", static_cast<natural_t>(t));
    //   return "";
    // }

    // bool render_pipeline_viewer::tag_picker_popup(const char* id, std::string& out) {
    //   bool committed = false;
    //   if (ImGui::BeginPopup(id)) {
    //     for (std::string_view known : kKnownTags) {
    //       if (ImGui::Selectable(known.data())) {
    //         out = known;
    //         committed = true;
    //         ImGui::CloseCurrentPopup();
    //       }
    //     }
    //     ImGui::Separator();

    //     if (ImGui::InputTextWithHint("##custom", "custom tag…", custom_tag_buf, sizeof custom_tag_buf, ImGuiInputTextFlags_EnterReturnsTrue)) {
    //       out = custom_tag_buf;
    //       custom_tag_buf[0] = '\0';
    //       committed = true;
    //       ImGui::CloseCurrentPopup();
    //     }
    //     ImGui::EndPopup();
    //   }
    //   return committed;
    // }

    // void render_pipeline_viewer::register_tag_string(resource_tag t, const std::string_view s) {}

  }  // namespace ui
}  // namespace other