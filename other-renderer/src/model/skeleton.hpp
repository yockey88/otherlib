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
#include "model/animation_limits.hpp"

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
    ///  carried through the palette it bounds the animated mesh
    bounding_box influenced_bounds = bounding_box::empty;
  };

  struct skeleton {
    std::string name;
    /// palette pre-multiplier: inverse(rigged mesh node global) * accumulated non-joint
    ///  ancestors of the first root joint; bind pose gives root_transform*bind_chain*inverse_bind == identity
    glm::mat4 root_transform{ 1.f };
    ostd::vector<joint> joints;                     // topologically ordered at import; capped kMaxBones with warning
    int16_t find_joint(natural_t name_hash) const;  // linear scan; joint counts are small
    bool empty() const { return joints.empty(); }
  };

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_SKELETON_HPP
