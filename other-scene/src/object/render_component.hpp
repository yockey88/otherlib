/**
 * \file object/render_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "model/model.hpp"
#include "renderer/gpu_structs.hpp"

#include "asset/asset.hpp"

namespace other {

  struct render_component {
    bool animated = false;
    bool visible = true;

    model obj_model;
    natural_t last_model_asset_id = 0;
    natural_t model_asset_id = 0;
    gpu::graphics_material material = {};
  };

  struct render_component_lua_proxy {
    render_component* native_pointer = nullptr;
  };

}  // namespace other

OTHER_REFLECT(
  other::render_component,
  field(animated, other::attr::serializable("Animated")),
  field(visible, other::attr::serializable("Visible")),
  field(obj_model, other::attr::serializable("Model")),
  field(model_asset_id, other::attr::serializable("Model"),
        other::attr::asset_identifier_field(other::asset::MODEL)))

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP