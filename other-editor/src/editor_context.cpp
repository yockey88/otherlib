/**
 * \file editor_context.cpp
 **/
#include "editor_context.hpp"

#include "editor_driver.hpp"

namespace other {

  namespace {

    /// a snapshot-pair edit: undo restores `before`, redo restores `after`. the scene is
    ///  resolved at invoke time through the context — never a captured scene pointer —
    ///  and the selection is carried across by name because restore reassigns runtime ids
    edit make_snapshot_edit(editor_context* ctx, ostd::vector<uint8_t> before, ostd::vector<uint8_t> after) {
      auto restore = [ctx](const ostd::vector<uint8_t>& snapshot) {
        scene* s = ctx->current_selection.scene_ptr;
        if (s == nullptr) {
          CORE_LOG_ERROR("Cannot restore scene edit: no active scene.");
          return;
        }

        std::vector<std::string> selected_names = {};
        for (const natural_t obj_id : ctx->current_selection.objects) {
          const scene_object* obj = s->find_object(obj_id);
          if (obj != nullptr) {
            selected_names.push_back(obj->name);
          }
        }

        s->restore_snapshot(snapshot);

        /// objects absent from the restored state (e.g. undoing a create) drop out
        ctx->current_selection.objects.clear();
        for (const std::string& obj_name : selected_names) {
          const scene_object* obj = s->find_object(obj_name);
          if (obj != nullptr && std::ranges::find(ctx->current_selection.objects, obj->id) == ctx->current_selection.objects.end()) {
            ctx->current_selection.objects.push_back(obj->id);
          }
        }

        ctx->edit_tracker.baseline = s->capture_snapshot();
      };
      return edit{
        .apply = [restore, after = std::move(after)]() { restore(after); },
        .undo = [restore, before = std::move(before)]() { restore(before); },
      };
    }

  }  // namespace

  void editor_context::notify_scene_edited() {
    scene* s = current_selection.scene_ptr;
    if (s == nullptr || s->is_playing()) {
      return;
    }

    if (!edit_tracker.session_active) {
      if (edit_tracker.baseline.empty()) {
        /// no baseline means we cannot reconstruct the pre-edit state; skip this session
        CORE_LOG_WARN("Scene edit began without a baseline snapshot; this edit will not be undoable.");
        edit_tracker.baseline = s->capture_snapshot();
        return;
      }
      edit_tracker.session_active = true;
    }
    edit_tracker.idle_frames = 0;
  }

  void editor_context::tick_edit_tracker() {
    scene* s = current_selection.scene_ptr;
    if (s == nullptr) {
      return;
    }

    /// keep a baseline ready from the first quiet frame so the next session has a
    ///  true pre-edit state to restore to
    if (!edit_tracker.session_active) {
      if (edit_tracker.baseline.empty() && !s->is_playing()) {
        edit_tracker.baseline = s->capture_snapshot();
      }
      return;
    }

    if (++edit_tracker.idle_frames < scene_edit_tracker::kIdleFramesToCommit) {
      return;
    }

    ostd::vector<uint8_t> after = s->capture_snapshot();
    editing_history.record(make_snapshot_edit(this, edit_tracker.baseline, after));
    edit_tracker.baseline = std::move(after);
    edit_tracker.session_active = false;
    edit_tracker.idle_frames = 0;
  }

  void editor_context::reset_scene_edit_tracking() {
    editing_history.clear();
    edit_tracker.baseline.clear();
    edit_tracker.session_active = false;
    edit_tracker.idle_frames = 0;
  }

  void editor_context::undo_scene_edit() {
    scene* s = current_selection.scene_ptr;
    if (s == nullptr || s->is_playing()) {
      return;
    }
    if (edit_tracker.session_active) {
      edit_tracker.idle_frames = scene_edit_tracker::kIdleFramesToCommit;
      tick_edit_tracker();
    }
    if (!editing_history.can_undo()) {
      CORE_LOG_DEBUG("Nothing to undo.");
      return;
    }
    editing_history.undo();
  }

  void editor_context::redo_scene_edit() {
    scene* s = current_selection.scene_ptr;
    if (s == nullptr || s->is_playing()) {
      return;
    }
    if (!editing_history.can_redo()) {
      CORE_LOG_DEBUG("Nothing to redo.");
      return;
    }
    editing_history.redo();
  }

  void editor_context::select_object(natural_t object_id) {
    if (current_selection.scene_ptr == nullptr) {
      CORE_LOG_ERROR("Cannot select object with ID {} because there is no active scene.", object_id);
      return;
    }

    if (!current_selection.multiple_selection_enabled) {
      current_selection.objects.clear();
    }

    current_selection.objects.push_back(object_id);
  }

  void editor_context::deselect_object(natural_t object_id) {
    if (current_selection.scene_ptr == nullptr) {
      CORE_LOG_ERROR("Cannot deselect object with ID {} because there is no active scene.", object_id);
      return;
    }

    auto& objects = current_selection.objects;
    auto it = std::ranges::find(objects, object_id);
    if (it != objects.end()) {
      objects.erase(it);
    } else {
      CORE_LOG_WARN("Tried to deselect object with ID {}, but it was not in the current selection.", object_id);
    }
  }

  bool editor_context::has_selection() const {
    return current_selection.scene_ptr != nullptr &&
      !current_selection.objects.empty();
  }

  bool editor_context::multi_select_enabled() const {
    return current_selection.multiple_selection_enabled;
  }

  bounding_box editor_context::get_selection_bounding_box() const {
    if (!has_selection()) {
      return {};
    }

    bounding_box result = bounding_box::empty;
    for (natural_t obj_id : current_selection.objects) {
      result = bounding_box::expand_to_include(result, current_selection.scene_ptr->get_bounding_box(obj_id));
    }

    if (result == bounding_box::empty) {
      result = bounding_box(glm::vec3(-1.f), glm::vec3(1.f));
    }
    return result;
  }

  natural_t editor_context::register_viewport(const std::string_view name, const std::string_view render_pipeline_name) {
    auto& vp = viewports.emplace_back() = viewport_info{
      .viewport_name = std::format("{}:{}", name, render_pipeline_name),
    };
    viewport_definition def = {
      .name = std::string(name),
      .render_pipeline_name = std::string(render_pipeline_name),
      .cam = &editor_camera,
    };

    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    vp.vp_id = rendering_sys.register_viewport(std::format("{}:{}", name, render_pipeline_name), def);
    return vp.vp_id;
  }

  natural_t editor_context::register_viewport(const std::string_view name, const std::string_view render_pipeline_name, camera* cam) {
    auto& vp = viewports.emplace_back() = viewport_info{
      .viewport_name = std::format("{}:{}", name, render_pipeline_name),
    };
    viewport_definition def = {
      .name = std::string(name),
      .render_pipeline_name = std::string(render_pipeline_name),
      .cam = cam,
    };

    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    vp.vp_id = rendering_sys.register_viewport(std::format("{}:{}", name, render_pipeline_name), def);
    return vp.vp_id;
  }

  void editor_context::remove_viewport(natural_t vp_id) {
    auto vp_itr = std::ranges::find_if(viewports, [vp_id](const viewport_info& vp) { return vp.vp_id == vp_id; });
    if (vp_itr != viewports.end()) {
      auto& kernel = driver->get_kernel();
      auto& rendering_sys = kernel.get_core_system<rendering_system>();
      rendering_sys.remove_viewport(vp_id);
      viewports.erase(vp_itr);
    } else {
      CORE_LOG_WARN("Tried to remove viewport with ID '{}', but no matching viewport was found.", vp_id);
    }
  }

  void editor_context::remove_all_viewports() {
    auto& kernel = driver->get_kernel();
    auto& rendering_sys = kernel.get_core_system<rendering_system>();
    for (const auto& vp : viewports) {
      if (scene_viewport_handle != 0 && vp.vp_id == scene_viewport_handle) {
        continue;
      }
      rendering_sys.remove_viewport(vp.vp_id);
    }
    viewports.clear();
  }

  viewport& editor_context::get_viewport(natural_t vp_id) {
    auto vp_itr = std::ranges::find_if(viewports, [vp_id](const viewport_info& vp) { return vp.vp_id == vp_id; });
    if (vp_itr != viewports.end()) {
      auto& kernel = driver->get_kernel();
      auto& rendering_sys = kernel.get_core_system<rendering_system>();
      return rendering_sys.get_viewport(vp_itr->vp_id);
    } else {
      static viewport null_viewport{
        .id = 0,
        .name = "null",
        .pipeline = nullptr,
        .cam = nullptr,
        .texture = {},
      };
      return null_viewport;
    }
  }

  ImTextureID editor_context::get_texture_id_by_name(const std::string_view name) const {
    if (name.empty()) {
      return 0;
    }

    return 0;
  }

  ImTextureID editor_context::get_pipeline_texture_id(const std::string_view pipeline_name, const std::string_view texture_name) const {
    if (pipeline_name.empty() || texture_name.empty()) {
      return 0;
    }

    auto* pl = driver->get_renderer().get_pipeline(std::string{ pipeline_name });
    if (pl == nullptr) {
      CORE_LOG_WARN("Pipeline '{}' not found when trying to get texture ID for texture '{}'", pipeline_name, texture_name);
      return 0;
    }

    return pl->get_texture_id(std::string{ texture_name });
  }

  ImVec2 editor_context::get_pipeline_texture_size(const std::string_view pipeline_name, const std::string_view texture_name) const {
    if (pipeline_name.empty() || texture_name.empty()) {
      return ImVec2(0.f, 0.f);
    }

    auto* pl = driver->get_renderer().get_pipeline(std::string{ pipeline_name });
    if (pl == nullptr) {
      CORE_LOG_WARN("Pipeline '{}' not found when trying to get texture size for texture '{}'", pipeline_name, texture_name);
      return ImVec2(0.f, 0.f);
    }

    glm::ivec2 size = pl->get_texture_size(std::string{ texture_name });
    return ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
  }

}  // namespace other