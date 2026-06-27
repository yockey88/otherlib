/**
 * \file ui/type-bindings/scene_ui.hpp
 **/
#ifndef OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP
#define OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP

#include <imgui/ImReflect.hpp>

#include "script/script_object.hpp"

#include "object/camera_component.hpp"
#include "object/light_component.hpp"
#include "object/physics_component.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"
#include "object/transform.hpp"
#include "scene/scene.hpp"

#include "ui/ui_node.hpp"
#include "ui/ui_window.hpp"


// IMGUI_REFLECT(other::scene, storage);

namespace other {
  namespace ui {

    struct scene_window : public ui_window {};

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_TYPE_BINDINGS_SCENE_UI_HPP