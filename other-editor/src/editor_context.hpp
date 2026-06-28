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
#include "editor_settings.hpp"

namespace other {

  class editor_driver;

  struct editor_context {
    struct selection {
      natural_t scene = 0;
      std::vector<natural_t> objects = {};
    };

    editor_driver* driver;
    editor_settings settings;

    scene* active_scene = nullptr;

    camera editor_camera;
    ImGuizmo::MODE gizmo_mode = ImGuizmo::MODE::LOCAL;
    ImGuizmo::OPERATION gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;

    edit_stack editing_history;
    selection current_selection;

    std::string current_viewport = "default-instancing";
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_CONTEXT_HPP