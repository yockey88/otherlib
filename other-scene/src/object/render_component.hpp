/**
 * \file object/render_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP

#include "core/defines.hpp"

#include "gpu_resource/shader.hpp"
#include "model/model.hpp"
#include "renderer/gpu_structs.hpp"

namespace other {

  struct render_component {
    bool animated = false;
    bool visible = true;

    model* model = nullptr;
    gpu::graphics_material material = {};
  };

}  // namespace other

OTHER_REFLECT(
  other::render_component,
  field(animated, other::attr::serializable()),
  field(visible, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP