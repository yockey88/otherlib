/**
 * \file renderer/material_layout.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_MATERIAL_LAYOUT_HPP
#define OTHER_RENDERER_RENDERER_MATERIAL_LAYOUT_HPP

#include <functional>
#include <span>

#include "core/defines.hpp"

#include "gpu_resource/material.hpp"

namespace other {

  /// material block layout a render pipeline declares in its TOML ([materials.layout]); pipeline
  ///  owns the GLSL/ABI, so offsets + element size are computed std430 here, not hand-maintained
  struct material_layout {
    struct param {
      std::string name;
      natural_t name_hash = 0;
      material_value::kind kind = material_value::kind::F32;
      uint32_t offset = 0;
      material_value default_value = {};
    };
    struct texture_slot {
      std::string name;
      natural_t name_hash = 0;
      std::string uniform;
      uint32_t unit = 0;
    };

    ostd::vector<param> params;
    ostd::vector<texture_slot> texture_slots;
    uint32_t element_size = 0;       //< std430 struct stride, one instance slot
    uint32_t instance_capacity = 100;  //< instance slots per draw block (kMaxMaterials today)
    opt<uint32_t> base_color_offset;   //< vec4 param named "base_color" — per-instance tint fold target

    /// computes std430 offsets (vec3 aligns to 16, scalars pack to 4) + element_size + base_color_offset;
    ///  layout declarations are engine-owned pipeline contracts, so malformed ones are fatal
    void finalize();

    /// reported once per (material, revision) because callers cache packs; receives the
    ///  offending param name when the material carries it
    using pack_warning_fn = std::function<void(std::string_view param_name, std::string_view reason)>;

    /// defaults first, then the material's values matched by name hash; unknown or
    ///  kind-mismatched params warn and are skipped. mat == nullptr packs pure defaults.
    void pack(const material* mat, std::span<uint8_t> out, const pack_warning_fn& warn = {}) const;

    /// multiplies the packed base_color of one element slot by a per-instance tint; no-op
    ///  when the layout declares no vec4 "base_color" param
    void fold_base_color_tint(std::span<uint8_t> element, const glm::vec4& tint) const;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_MATERIAL_LAYOUT_HPP
