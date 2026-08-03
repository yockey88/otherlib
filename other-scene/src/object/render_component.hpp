/**
 * \file object/render_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "model/model.hpp"

#include "asset/asset.hpp"

namespace other {

  struct render_component {
    bool visible = true;

    model obj_model;
    natural_t last_model_asset_id = 0;
    natural_t model_asset_id = 0;

    /// material asset override; 0 = the model's own imported materials (per submesh), and a
    ///  material the active pipeline's layout doesn't fully know still renders — unknown
    ///  params are skipped, missing ones take the layout defaults
    natural_t last_material_asset_id = 0;
    natural_t material_asset_id = 0;

    /// per-instance multiplier folded into the packed base_color at bind time — the
    ///  per-object-color use case without a material asset
    glm::vec4 tint = glm::vec4(1.f);
  };

  struct render_component_lua_proxy {
    render_component* native_pointer = nullptr;
  };

}  // namespace other

OTHER_REFLECT(
  other::render_component,
  field(visible, other::attr::serializable("Visible")),
  /// runtime-derived (produce_model regenerates it from the source), never persisted
  field(obj_model, other::attr::serializable("Model"), other::attr::native_only()),
  field(tint, other::attr::serializable("Tint")),
  /// holds the model SOURCE asset id; per-instance variation is the material system's job
  field(model_asset_id, other::attr::serializable("Model"),
        other::attr::asset_identifier_field(other::asset::MODEL_SOURCE)),
  field(material_asset_id, other::attr::serializable("Material"),
        other::attr::asset_identifier_field(other::asset::MATERIAL)))

#endif  // OTHER_SCENE_OBJECT_RENDER_COMPONENT_HPP
