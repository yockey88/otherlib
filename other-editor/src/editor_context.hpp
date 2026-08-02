/**
 * \file editor_context.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_CONTEXT_HPP
#define OTHER_EDITOR_EDITOR_CONTEXT_HPP

#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>

#include "renderer/camera.hpp"

#include "scene/scene.hpp"

#include "edit_stack.hpp"
#include "editor_selection.hpp"
#include "editor_settings.hpp"

namespace other {

  class editor_driver;

  struct editor_context {
    struct viewport_info {
      natural_t vp_id;
      std::string viewport_name;
    };

    editor_driver* driver;
    editor_settings settings;

    scene* active_scene = nullptr;
    camera* scene_active_camera = nullptr;
    natural_t scene_viewport_handle = 0;

    camera editor_camera;
    ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;
    ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

    edit_stack editing_history;

    /// snapshot-based scene edit coalescing: continuous widget edits (inspector drags)
    ///  open a session on the first notify and commit one {baseline, after} snapshot
    ///  pair once the widgets go quiet; discrete ops use commit_discrete_scene_edit
    struct scene_edit_tracker {
      constexpr static uint32_t kIdleFramesToCommit = 12;

      ostd::vector<uint8_t> baseline = {};
      bool session_active = false;
      uint32_t idle_frames = 0;
    } edit_tracker;

    /// a scene-mutating widget changed this frame (no-op while the scene plays)
    void notify_scene_edited();
    /// once per frame: commits the open edit session after the widgets go quiet
    void tick_edit_tracker();
    /// re-baselines and clears history (scene switched)
    void reset_scene_edit_tracking();

    void undo_scene_edit();
    void redo_scene_edit();

    selection current_selection;

    std::string current_primary_viewport = "default-instancing";
    std::vector<viewport_info> viewports;

    void select_object(natural_t object_id);
    void deselect_object(natural_t object_id);

    bool has_selection() const;
    bool multi_select_enabled() const;
    bounding_box get_selection_bounding_box() const;

    natural_t register_viewport(const std::string_view name, const std::string_view render_pipeline_name);
    natural_t register_viewport(const std::string_view name, const std::string_view render_pipeline_name, camera* cam);
    void remove_viewport(natural_t vp_id);
    void remove_all_viewports();

    viewport& get_viewport(natural_t vp_id);

    ImTextureID get_texture_id_by_name(const std::string_view name) const;
    ImTextureID get_pipeline_texture_id(const std::string_view pipeline_name, const std::string_view texture_name) const;

    ImVec2 get_pipeline_texture_size(const std::string_view pipeline_name, const std::string_view texture_name) const;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_CONTEXT_HPP