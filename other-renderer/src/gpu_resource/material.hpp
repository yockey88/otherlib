/**
 * \file gpu_resource/material.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_MATERIAL_HPP
#define OTHER_RENDERER_GPU_RESOURCE_MATERIAL_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"

namespace other {

  /// layout-agnostic bag of typed params + texture-slot paths; packing against a
  ///  pipeline-declared material_layout happens late, at bind time (render_pipeline)
  struct material_value {
    enum class kind : uint8_t { F32, VEC2, VEC3, VEC4, I32, B32 };
    kind value_kind = kind::F32;
    glm::vec4 data = glm::vec4(0.f);  //< scalars in .x, ints/bools bit-cast into .x

    static material_value from(float v) { return { kind::F32, glm::vec4(v, 0.f, 0.f, 0.f) }; }
    static material_value from(const glm::vec2& v) { return { kind::VEC2, glm::vec4(v, 0.f, 0.f) }; }
    static material_value from(const glm::vec3& v) { return { kind::VEC3, glm::vec4(v, 0.f) }; }
    static material_value from(const glm::vec4& v) { return { kind::VEC4, v }; }
    static material_value from(int32_t v) {
      material_value mv{ kind::I32 };
      std::memcpy(&mv.data.x, &v, sizeof(v));
      return mv;
    }
    static material_value from(bool v) {
      material_value mv{ kind::B32 };
      const int32_t iv = v ? 1 : 0;
      std::memcpy(&mv.data.x, &iv, sizeof(iv));
      return mv;
    }
  };

  struct material {
    std::string name;
    ostd::map<natural_t, material_value> params;      //< FNV(param name) -> value
    ostd::map<natural_t, std::string> param_names;    //< FNV(param name) -> name, kept for diagnostics
    ostd::map<natural_t, std::string> texture_paths;  //< FNV(slot name) -> as-authored path ("" = none)
    ostd::map<natural_t, natural_t> texture_hashes;   //< FNV(slot name) -> texture asset path_hash (resolved at load)

    /// registry identity + revision, stamped by renderer_backend when registered;
    ///  pipeline pack caches key off (key, revision) so reloads repack and reuse never aliases
    natural_t key = 0;
    uint32_t revision = 0;
  };

  /// scene_parse_result discipline: files are data, so malformed input is an error result with
  ///  a message plus non-fatal warnings — never an assert
  struct material_parse_result {
    opt<material> mat;
    std::string error;
    ostd::vector<std::string> warnings;

    bool success() const { return mat.has_value(); }
  };

  /// pure toml parse, engine-free (no subsystems); texture slot paths come back verbatim —
  ///  resolving them against the file's directory is the loader's job
  material_parse_result parse_material_toml(const filepath& path);

}  // namespace other

#endif  // OTHER_RENDERER_GPU_RESOURCE_MATERIAL_HPP
