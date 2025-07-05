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
    model* model = nullptr;
    shader* shader_handle = nullptr;
    gpu::graphics_material material = {};
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP