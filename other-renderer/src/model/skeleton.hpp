/**
 * \file model/skeleton.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_SKELETON_HPP
#define OTHER_RENDERER_MODEL_SKELETON_HPP

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/defines.hpp"
#include "math/bounding_box.hpp"

namespace other {

  struct joint {
    natural_t name_hash = 0;  // FNV(name) — clip tracks bind by this
    std::string name;
    int16_t parent = -1;             // index into joints; -1 = root. parents ALWAYS precede children
    glm::mat4 inverse_bind{ 1.f };   // aiBone::mOffsetMatrix
    glm::vec3 bind_position{ 0.f };  // decomposed node local bind TRS (the pose fallback)
    glm::quat bind_rotation{ 1.f, 0.f, 0.f, 0.f };
    glm::vec3 bind_scale{ 1.f };
    /// bind-space AABB of the vertices this joint influences (empty for helper joints);
    ///  carried through the palette it bounds the ANIMATED mesh (scene::get_bounding_box)
    bounding_box influenced_bounds = bounding_box::empty;
  };

  struct skeleton {
    std::string name;
    /// accumulated transform of every NON-joint ancestor above the first skeleton root
    ///  (scene root included) — the palette pre-multiplier. inverse_bind matrices invert the
    ///  FULL global bind chain, so build_palette needs this prefix back in front for
    ///  bind pose to reproduce the raw mesh (axis/unit fixes often live on these nodes)
    glm::mat4 root_transform{ 1.f };
    ostd::vector<joint> joints;                     // topologically ordered at import; capped kMaxBones with warning
    int16_t find_joint(natural_t name_hash) const;  // linear scan; joint counts are small
    bool empty() const { return joints.empty(); }
  };

  /// bone_matrix_buffer and the MAX_BONES shader define both derive from this
  constexpr inline size_t kMaxBones = 100;

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_SKELETON_HPP
