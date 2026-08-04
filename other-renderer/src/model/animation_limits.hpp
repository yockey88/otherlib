/**
 * \file model/animation_limits.hpp
 **/
#ifndef OTHER_RENDERER_MODEL_ANIMATION_LIMITS_HPP
#define OTHER_RENDERER_MODEL_ANIMATION_LIMITS_HPP

#include <cstddef>

namespace other {

  /// bone_matrix_buffer and the MAX_BONES shader define both derive from this
  constexpr inline size_t kMaxBones = 100;

}  // namespace other

#endif  // OTHER_RENDERER_MODEL_ANIMATION_LIMITS_HPP
