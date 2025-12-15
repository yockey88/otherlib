/**
 * \file object/render_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "gpu_resource/shader.hpp"
#include "model/model.hpp"
#include "renderer/gpu_structs.hpp"

namespace other {

  struct render_component {
    bool animated = false;
    bool visible = true;

    model obj_model;
    natural_t model_asset_id = 0;
    std::vector<uint32_t> submesh_indices = {};
    gpu::graphics_material material = {};
  };

}  // namespace other

OTHER_REFLECT(
  other::render_component,
  field(animated, other::attr::serializable()),
  field(visible, other::attr::serializable()),
  field(obj_model, other::attr::serializable()),
  field(model_asset_id, other::attr::serializable()),
  field(submesh_indices, other::attr::serializable())
)

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP