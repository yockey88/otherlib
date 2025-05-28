/**
 * \file simulation/scene_object.hpp
 **/
#ifndef OTHER_SIMULATION_SCENE_OBJECT_HPP
#define OTHER_SIMULATION_SCENE_OBJECT_HPP

#include "core/ref.hpp"

#include "math/ray.hpp"
#include "simulation/material.hpp"

namespace other {

  struct intersection_info {
    bool hit = false;
    bool out_face = false;
    orthonormal_basis local_basis;

    glm::vec3 hit_point;
    glm::vec3 normal;
    float t;

    other::ref<material> mat = nullptr;

    void set_face_normal(const ray& r, const glm::vec3& outward_normal);
  };

  class scene_object : public other::ref_counted {
   public:
    scene_object() = default;
    virtual ~scene_object() = default;

    void intersect(const ray& r, const interval& range, intersection_info& info) const;

   protected:
    virtual void ray_intersect(const ray& r, const interval& range, intersection_info& info) const = 0;

    void set_material(const other::ref<material>& m);

   private:
    other::ref<material> mat = nullptr;
  };

  struct compound_object : public scene_object {
    std::vector<other::ref<scene_object>> children;

    ~compound_object() override;

    template <typename T, typename... Args>
      requires std::is_base_of_v<scene_object, T>
    void add_child(Args&&... args) {
      children.push_back(other::ref<T>::create(std::forward<Args>(args)...));
    }

    void ray_intersect(const ray& r, const interval& range, intersection_info& info) const override;
  };

}  // namespace other

#endif  // OTHER_SIMULATION_SCENE_OBJECT_HPP