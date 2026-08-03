/**
 * \file model/skeleton.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_SKELETON_HPP
#define OTHER_RENDERER_MODEL_SKELETON_HPP

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/defines.hpp"

namespace other {

  struct joint {
    natural_t name_hash = 0;  // FNV(name) — clip tracks bind by this
    std::string name;
    int16_t parent = -1;             // index into joints; -1 = root. parents ALWAYS precede children
    glm::mat4 inverse_bind{ 1.f };   // aiBone::mOffsetMatrix
    glm::vec3 bind_position{ 0.f };  // decomposed node local bind TRS (the pose fallback)
    glm::quat bind_rotation{ 1.f, 0.f, 0.f, 0.f };
    glm::vec3 bind_scale{ 1.f };
  };

  struct skeleton {
    std::string name;
    glm::mat4 global_inverse{ 1.f };                // scene root inverse (today's inverse_global_transform use)
    ostd::vector<joint> joints;                     // topologically ordered at import; capped kMaxBones with warning
    int16_t find_joint(natural_t name_hash) const;  // linear scan; joint counts are small
    bool empty() const { return joints.empty(); }
  };

  /// bone_matrix_buffer and the MAX_BONES shader define both derive from this
  constexpr inline size_t kMaxBones = 100;

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_SKELETON_HPP
