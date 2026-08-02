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

/// reflected here rather than in gpu_structs.hpp so the gpu header stays free of the
///  reflection machinery; this is the only reflection surface for the material
OTHER_REFLECT(
  other::gpu::graphics_material,
  field(diffuse_color, other::attr::serializable("Diffuse Color")),
  field(diffuse_reflectivity, other::attr::serializable("Diffuse Reflectivity")),
  field(specular_color, other::attr::serializable("Specular Color")),
  field(specular_reflectivity, other::attr::serializable("Specular Reflectivity")),
  field(emissive_color, other::attr::serializable("Emissive Color")),
  field(emissivity, other::attr::serializable("Emissivity")),
  field(transparency, other::attr::serializable("Transparency")),
  field(shininess, other::attr::serializable("Shininess")))

OTHER_REFLECT(
  other::render_component,
  field(animated, other::attr::serializable("Animated")),
  field(visible, other::attr::serializable("Visible")),
  /// runtime-derived (produce_model regenerates it from the source), never persisted
  field(obj_model, other::attr::serializable("Model"), other::attr::native_only()),
  field(material, other::attr::serializable("Material")),
  /// holds the model SOURCE asset id; per-instance variation (materials) is the material
  ///  system's job, not a model-copy's
  field(model_asset_id, other::attr::serializable("Model"),
        other::attr::asset_identifier_field(other::asset::MODEL_SOURCE)))

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP