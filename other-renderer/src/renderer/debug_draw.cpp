/**
 * \file renderer/debug_draw.cpp
 **/
#include "renderer/debug_draw.hpp"

#include "renderer/debug_render_stream.hpp"

namespace other {

  void debug_draw::line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }

    sink->submit(builtin_debug_streams::kLines, debug_vertex{ a, c });
    sink->submit(builtin_debug_streams::kLines, debug_vertex{ b, c });
  }
  void debug_draw::triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& d, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }

    sink->submit(builtin_debug_streams::kTris, debug_vertex{ a, c });
    sink->submit(builtin_debug_streams::kTris, debug_vertex{ b, c });
    sink->submit(builtin_debug_streams::kTris, debug_vertex{ d, c });
  }
  void debug_draw::point(const glm::vec3& p, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }

    sink->submit(builtin_debug_streams::kPoints, debug_vertex{ p, c });
  }

  void debug_draw::ray(const glm::vec3& o, const glm::vec3& dir, float len, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }
    arrow(o, o + glm::normalize(dir) * len, c);
  }

  void debug_draw::arrow(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }
  }

  void debug_draw::aabb(const bounding_box& b, const glm::vec4& c) {
    aabb(b.min, b.max, c);
  }

  void debug_draw::aabb(const glm::vec3& mn, const glm::vec3& mx, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }

    const glm::vec3 v[8] = {
      { mn.x, mn.y, mn.z },
      { mx.x, mn.y, mn.z },
      { mx.x, mx.y, mn.z },
      { mn.x, mx.y, mn.z },
      { mn.x, mn.y, mx.z },
      { mx.x, mn.y, mx.z },
      { mx.x, mx.y, mx.z },
      { mn.x, mx.y, mx.z },
    };
    constexpr int e[12][2] = {
      { 0, 1 },
      { 1, 2 },
      { 2, 3 },
      { 3, 0 },
      { 4, 5 },
      { 5, 6 },
      { 6, 7 },
      { 7, 4 },
      { 0, 4 },
      { 1, 5 },
      { 2, 6 },
      { 3, 7 },
    };
    for (auto& pr : e) {
      line(v[pr[0]], v[pr[1]], c);
    }
  }

  void debug_draw::obb(const glm::mat4& xform, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }
  }

  void debug_draw::sphere(const glm::vec3& center, float r, const glm::vec4& c, int rings) {
    if (sink == nullptr) {
      return;
    }
  }

  void debug_draw::frustum(const glm::mat4& inv_view_proj, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }
  }

  void debug_draw::transform(const glm::mat4& m, float scale) {
    if (sink == nullptr) {
      return;
    }

    const glm::vec3 o = glm::vec3(m[3]);
    line(o, o + scale * glm::vec3(m[0]), debug_colors::kRed);
    line(o, o + scale * glm::vec3(m[1]), debug_colors::kGreen);
    line(o, o + scale * glm::vec3(m[2]), debug_colors::kBlue);
  }

  void debug_draw::mesh(resource_handle h, const glm::mat4& model, const glm::vec4& tint, bool wireframe) {
    if (sink == nullptr) {
      return;
    }
    // sink->submit(builtin_debug_streams::kMeshes, debug_mesh_instance{ .mesh_handle = h, .model = model, .color = tint, .submesh_index = wireframe ? 1u : 0u });
  }

}  // namespace other