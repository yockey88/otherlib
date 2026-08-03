/**
 * \file model/skeleton.cpp
 **/
#include "model/skeleton.hpp"

namespace other {

  int16_t skeleton::find_joint(natural_t name_hash) const {
    for (size_t i = 0; i < joints.size(); ++i) {
      if (joints[i].name_hash == name_hash) {
        return static_cast<int16_t>(i);
      }
    }
    return -1;
  }

}  // namespace other
