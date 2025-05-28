/**
 * \file math/integrator.cpp
 **/
#include "math/integrator.hpp"

namespace other {

  glm::vec3 test_sampler::sample(const ray& r, const glm::ivec2& pixel) const {
    glm::vec3 color = black;
    switch (pixel.x % 3) {
      case 0:
        color.r = 1.f;
        break;

      case 1:
        color.g = 1.f;
        break;

      case 2:
        color.b = 1.f;
        break;
    }
    return color;
  }

}  // namespace other