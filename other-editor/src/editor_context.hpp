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
    editor_driver* driver;
    editor_settings settings;

    scene* active_scene = nullptr;
    natural_t scene_render_pl_handle = 0;

    camera editor_camera;
    ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;
    ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

    edit_stack editing_history;

    selection current_selection;

    std::string current_viewport = "default-instancing";

    void select_object(natural_t object_id);
    bool has_selection() const;
    bool multi_select_enabled() const;
    bounding_box get_selection_bounding_box() const;

    ImTextureID get_texture_id_by_name(const std::string_view name) const;
    ImTextureID get_pipeline_texture_id(const std::string_view pipeline_name, const std::string_view texture_name) const;

    ImVec2 get_pipeline_texture_size(const std::string_view pipeline_name, const std::string_view texture_name) const;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_CONTEXT_HPP