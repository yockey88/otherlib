/**
 * \file ui/render-pipeline-ui/render_pipeline_editor.cpp
 **/
#include "ui/render-pipeline-ui/render_pipeline_editor.hpp"

#include "core/profiler.hpp"
#include "theme/colors.hpp"
#include "ui/inspector_widgets.hpp"
#include "ui/unicode.hpp"

#include "editor_driver.hpp"

namespace other {
  namespace ui {

    render_pipeline_editor::render_pipeline_editor(editor_context& ctx, event_system& events)
        : ui_window(&events, "Render Pipeline Editor"), editor_ctx(ctx) {
    }

    void render_pipeline_editor::on_render_header() {
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

    void render_pipeline_editor::on_render_body() {
      PROFILE_SECTION("render_pipeline_editor::on_render_body");
      draw_list();
      draw_validation_banner();
      draw_properties();
      draw_node_editor();

      // draw_required_tags();
      // draw_chain_section();
    }

    void render_pipeline_editor::draw_list() {
      PROFILE_SECTION("render_pipeline_editor::draw_list");
      entries = inspector::rebuild_render_pipeline_list(editor_ctx.driver->get_renderer(), editor_ctx.driver->get_kernel().get_core_system<asset_system>());
      uint32_t new_idx = inspector::draw_render_pipeline_list(entries, selected, pending_select);

      const bool current = new_idx == selected;
      if (current && !is_dirty) {
        clear_working();
      } else if (current && is_dirty) {
        pending_select = new_idx;
        ImGui::OpenPopup("confirm-discard");
      } else if (!current && new_idx >= 0 && new_idx < entries.size()) {
        selected = new_idx;
        load_into_working(entries[selected].name);
      }
      draw_discard_confirm_popup();
    }

    void render_pipeline_editor::draw_validation_banner() {
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

    void render_pipeline_editor::draw_properties() {
      PROFILE_SECTION("render_pipeline_editor::draw_properties");
      if (selected < 0 || selected >= entries.size()) {
        ImGui::TextDisabled("No pipeline selected.");
        return;
      }

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

        if (draw_texture_name_combo("Display", working_def.display_texture_name)) {
          mark_dirty();
        }
        inspector::end_pipeline_properties();
      }
    }

    void render_pipeline_editor::draw_node_editor() {
      PROFILE_SECTION("render_pipeline_editor::draw_node_editor");
      if (!has_doc) {
        ImGui::TextDisabled("No pipeline loaded.");
        return;
      }

      struct resource_pin {
        pipeline_resource_reference* ref;
        natural_t pin_id;
      };
      struct editor_node {
        std::vector<resource_pin> input_textures;
        std::vector<resource_pin> input_buffers;
        std::vector<resource_pin> output_textures;
        std::vector<resource_pin> output_buffers;
      };
      struct graph {
        ostd::map<natural_t, editor_node> nodes;
        ostd::map<natural_t, std::set<natural_t>> edges;
      };
      graph g;

      working_editor.begin(working_def.name);
      ostd::map<natural_t, pipeline_pass_definition*> node_to_pass;
      for (auto& p : working_def.passes) {
        natural_t id = working_editor.begin_node(p.name);
        node_to_pass[id] = &p;

        auto& n = g.nodes.emplace(id, editor_node{}).first->second;

        auto input_buffers = p.inputs |
          std::views::filter([](const pipeline_resource_reference& r) { return r.type == resource_type::BUFFER; }) |
          std::views::transform([](pipeline_resource_reference& r) { return &r; }) |
          std::ranges::to<std::vector>();
        auto output_buffers = p.outputs |
          std::views::filter([](const pipeline_resource_reference& r) { return r.type == resource_type::BUFFER; }) |
          std::views::transform([](pipeline_resource_reference& r) { return &r; }) |
          std::ranges::to<std::vector>();

        auto input_textures = p.inputs |
          std::views::filter([](const pipeline_resource_reference& r) { return r.type == resource_type::TEXTURE; }) |
          std::views::transform([](pipeline_resource_reference& r) { return &r; }) |
          std::ranges::to<std::vector>();
        auto output_textures = p.outputs |
          std::views::filter([](const pipeline_resource_reference& r) { return r.type == resource_type::TEXTURE; }) |
          std::views::transform([](pipeline_resource_reference& r) { return &r; }) |
          std::ranges::to<std::vector>();

        for (auto& input : input_buffers) {
          natural_t p = working_editor.begin_input_pin(input->resource_name);
          n.input_buffers.push_back({ input, p });
          working_editor.end_pin();
        }
        for (auto& input : input_textures) {
          natural_t p = working_editor.begin_input_pin(input->resource_name);
          n.input_textures.push_back({ input, p });
          working_editor.end_pin();
        }

        for (auto& output : output_buffers) {
          natural_t p = working_editor.begin_output_pin(output->resource_name);
          n.output_buffers.push_back({ output, p });
          working_editor.end_pin();
        }
        for (auto& output : output_textures) {
          natural_t p = working_editor.begin_output_pin(output->resource_name);
          n.output_textures.push_back({ output, p });
          working_editor.end_pin();
        }

        working_editor.end_node();
      }

      for (const auto& n1 : g.nodes) {
        auto& e1 = g.edges[n1.first];

        for (const auto& n2 : g.nodes) {
          if (n1.first == n2.first) {
            continue;
          }
          auto& e2 = g.edges[n2.first];

          for (const auto& res : n1.second.output_textures) {
            if (auto itr = std::ranges::find_if(n2.second.input_textures, [&](const auto& r) -> bool { return r.ref->resource_name == res.ref->resource_name; });
                itr != n2.second.input_textures.end() &&  // if n2 reads from a texture that n1 writes to
                !e2.contains(n1.first)) {                 // and there is not already a backwards edge from n2 to n1
              e1.insert(n2.first);
            }
          }
          for (const auto& res : n1.second.output_buffers) {
            if (auto itr = std::ranges::find_if(n2.second.input_buffers, [&](const auto& r) -> bool { return r.ref->resource_name == res.ref->resource_name; });
                itr != n2.second.input_buffers.end() &&  // if n2 reads from a buffer that n1 writes to
                !e2.contains(n1.first)) {                // and there is not already a backwards edge from n2 to n1
              e1.insert(n2.first);
            }
          }
        }
      }

      for (const auto& [nid, e] : g.edges) {
        for (const auto& to_nid : e) {
          auto& from_node = g.nodes.at(nid);
          auto& to_node = g.nodes.at(to_nid);

          for (const auto& from_res : from_node.output_textures) {
            if (auto itr = std::ranges::find_if(to_node.input_textures, [&](const auto& r) -> bool { return r.ref->resource_name == from_res.ref->resource_name; });
                itr != to_node.input_textures.end()) {
              working_editor.link(from_res.pin_id, itr->pin_id);
            }
          }
          for (const auto& from_res : from_node.output_buffers) {
            if (auto itr = std::ranges::find_if(to_node.input_buffers, [&](const auto& r) -> bool { return r.ref->resource_name == from_res.ref->resource_name; });
                itr != to_node.input_buffers.end()) {
              working_editor.link(from_res.pin_id, itr->pin_id);
            }
          }
        }
      }

      working_editor.end();
    }
    /*
    std::vector<frame_node> nodes;
    nodes.reserve(passes.size());

    for (auto& [id, pass] : passes) {
      auto& n = nodes.emplace_back() = frame_node{
        .id = pass.pass.id,
        .pass = &pass.pass,
      };

      for (const auto& [id, texture] : pass.pass.texture_resources) {
        if (texture.flags & READ || texture.flags & SAMPLE) {
          n.input_textures.insert({ id, texture });
        }
        if (texture.flags & WRITE) {
          n.output_textures.insert({ id, texture });
        }
      }

      for (const auto& [id, buffer] : pass.pass.buffer_resources) {
        if (buffer.flags & READ) {
          n.input_buffers.insert({ id, buffer });
        }
        if (buffer.flags & WRITE) {
          n.output_buffers.insert({ id, buffer });
        }
      }
    }

    /// list of outgoing edges
    ostd::map<natural_t, std::set<natural_t>> edges;
    for (const auto& n1 : nodes) {
      auto& e1 = edges[n1.id];

      for (const auto& n2 : nodes) {
        if (n1 == n2) {
          continue;
        }
        auto& e2 = edges[n2.id];

        for (const auto& [slot, texture] : n1.output_textures) {
          if (auto itr = std::ranges::find_if(n2.input_textures, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; });
              itr != n2.input_textures.end() &&  // if n2 reads from a texture that n1 writes to
              !e2.contains(n1.id)) {             // and there is not already a backwards edge from n2 to n1
            e1.insert(n2.id);
          }
        }
        for (const auto& [slot, texture] : n1.output_buffers) {
          if (auto itr = std::ranges::find_if(n2.input_buffers, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; });
              itr != n2.input_buffers.end() &&  // if n2 reads from a buffer that n1 writes to
              !e2.contains(n1.id)) {            // and there is not already a backwards edge from n2 to n1
            e1.insert(n2.id);
          }
        }
      }
    }

    for (const auto& n : nodes) {
      for (const auto& depends_on_str : n.pass->depends_on) {
        auto itr = std::ranges::find_if(nodes, [&](const frame_node& node) -> bool { return node.pass->name == depends_on_str; });
        if (itr == nodes.end()) {
          continue;
        }

        auto& e = edges[itr->id];
        if (!e.contains(n.id)) {
          e.insert(n.id);
        }
      }
    }

    for (natural_t i = 0; i < nodes.size(); ++i) {
      const auto& n = nodes[i];
      pass_graph.nodes.insert({ n.id, n });
    }
    pass_graph.edges = std::move(edges);
    */
    void render_pipeline_editor::draw_discard_confirm_popup() {
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

    bool render_pipeline_editor::draw_texture_name_combo(const char* label, std::string& texture_name) {
      bool changed = false;
      inspector::begin_property_row(label);
      if (ImGui::BeginCombo("##tex", texture_name.empty() ? "<none>" : texture_name.c_str())) {
        if (ImGui::Selectable("<none>", texture_name.empty())) {
          texture_name.clear();
          changed = true;
        }
        for (const pipeline_texture_definition& t : working_def.textures) {
          if (ImGui::Selectable(t.name.c_str(), t.name == texture_name)) {
            texture_name = t.name;
            changed = true;
          }
        }
        ImGui::EndCombo();
      }
      inspector::end_property_row();
      return changed;
    }

    void render_pipeline_editor::load_into_working(const std::string_view pipeline_name) {
      PROFILE_SECTION("render_pipeline_editor::load_into_working");
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

    void render_pipeline_editor::clear_working() {
      working_def = pipeline_definition{};
      has_doc = false;
      selected = -1;
    }

    detail::validation_result render_pipeline_editor::validate_pipeline_definition(const pipeline_definition& def) const {
      detail::validation_result result;

      if (def.name.empty()) {
        result.errors.push_back("Pipeline must have a name.");
      }

      // if (def.display_texture_name.empty()) {
      //   result.errors.push_back("Pipeline must specify a display texture.");
      // } else if (std::ranges::none_of(def.textures, [&](const pipeline_texture_definition& t) { return t.name == def.display_texture_name; })) {
      //   result.errors.push_back(std::format("Display texture '{}' not found in pipeline textures.", def.display_texture_name));
      // }

      result.valid = result.errors.empty();
      return result;
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

  }  // namespace ui
}  // namespace other