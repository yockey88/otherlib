/**
 * \file math/coordinate_frame.hpp
 **/
#ifndef OTHER_CORE_MATH_COORDINATE_FRAME_HPP
#define OTHER_CORE_MATH_COORDINATE_FRAME_HPP

#include "math/orthonormal_basis.hpp"

namespace other {

  class coordinate_frame {
   public:
    coordinate_frame(const orthonormal_basis& basis)
        : basis(basis) {}

   private:
    orthonormal_basis basis;
  };

}  // namespace other

#endif  // !OTHER_CORE_MATH_COORDINATE_FRAME_HPP