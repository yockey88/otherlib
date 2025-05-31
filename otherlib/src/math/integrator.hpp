/**
 * \file math/integrator.hpp
 **/
#ifndef OTHER_MATH_INTEGRATOR_HPP
#define OTHER_MATH_INTEGRATOR_HPP

#include <string>

#include <glm/glm.hpp>

#include "math/ray.hpp"

namespace other {

  class integrator {
   public:
    virtual ~integrator() = default;

    virtual std::string to_string() const = 0;
    virtual void render() = 0;

    // pstd::optional<ShapeIntersection> Intersect(const Ray& ray, real_t tMax = Infinity) const;
    // bool IntersectP(const Ray& ray, real_t tMax = Infinity) const;
    // bool Unoccluded(const Interaction& p0, const Interaction& p1) const {
    //   return !IntersectP(p0.SpawnRayTo(p1), 1 - ShadowEpsilon);
    // }
    // SampledSpectrum Tr(const Interaction& p0, const Interaction& p1, const SampledWavelengths& lambda) const;

   private:
  };

  class sampler {
   public:
    virtual ~sampler() = default;

    virtual glm::vec3 sample(const ray& r, const glm::ivec2& pixel) const = 0;
  };

  class test_sampler : public sampler {
   public:
    test_sampler() = default;

    glm::vec3 sample(const ray& r, const glm::ivec2& pixel) const override;

    glm::vec3 white = { 1.f, 1.f, 1.f };
    glm::vec3 black = { 0.f, 0.f, 0.f };
  };

}  // namespace other

#endif  // OTHER_MATH_INTEGRATOR_HPP
