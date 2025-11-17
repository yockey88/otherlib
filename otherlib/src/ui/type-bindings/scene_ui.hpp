/**
 * \file ui/type-bindings/scene_ui.hpp
 **/
#ifndef OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP
#define OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP

#include <imgui/ImReflect.hpp>

#include "renderer/ui/ui_node.hpp"
#include "renderer/ui/ui_window.hpp"
#include "script/script_object.hpp"

#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

IMGUI_REFLECT(other::script_object, name, id);
IMGUI_REFLECT(other::script_component, object, script_object_id);
IMGUI_REFLECT(other::scene_object, id, registry_id, name, visible);
// IMGUI_REFLECT(other::scene, storage);

namespace other {
  namespace ui {

    struct scene_window : public ui_window {};

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP