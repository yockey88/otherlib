/**
 * \file simulation/scene_object.cpp
 **/
#include "simulation/scene_object.hpp"

namespace other {

  void intersection_info::set_face_normal(const ray& r, const glm::vec3& outward_normal) {
    out_face = glm::dot(r.direction, outward_normal) < 0.f;
    normal = out_face ? outward_normal : -outward_normal;
    local_basis = orthonormal_basis(normal);
  }

  void scene_object::intersect(const ray& r, const interval& range, intersection_info& info) const {
    ray_intersect(r, range, info);
    if (info.hit) {
      info.mat = mat;
    }
  }

  void scene_object::set_material(const other::ref<material>& m) {
    mat = m;
  }

  compound_object::~compound_object() {
    children.clear();
  }

  void compound_object::ray_intersect(const ray& r, const interval& range, intersection_info& info) const {
    intersection_info temp_info;
    bool hit_any = false;
    float closest_t = range.max;

    for (const auto& child : children) {
      intersection_info child_info;
      child->intersect(r, { range.min, closest_t }, child_info);

      if (child_info.hit) {
        hit_any = true;
        closest_t = child_info.t;
        temp_info = child_info;
      }
    }

    if (hit_any) {
      info.hit = true;
      info = temp_info;
    }
  }

}  // namespace other