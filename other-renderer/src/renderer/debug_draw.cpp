/**
 * \file renderer/debug_draw.cpp
 **/
#include "renderer/debug_draw.hpp"

#include "renderer/render_stream.hpp"

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

    // shaft
    line(a, b, c);

    const glm::vec3 dir = b - a;
    const float len = glm::length(dir);
    if (len < 1e-5f) {
      return;
    }

    const glm::vec3 fwd = dir / len;
    const glm::vec3 ref = (glm::abs(fwd.y) < 0.99f) ?
      glm::vec3(0, 1, 0) :
      glm::vec3(1, 0, 0);
    const glm::vec3 right = glm::normalize(glm::cross(fwd, ref));
    const glm::vec3 up = glm::cross(right, fwd);

    const float head = glm::min(0.15f * len, 0.5f);
    const float wid = head * 0.5f;
    const glm::vec3 base = b - fwd * head;

    line(b, base + right * wid, c);
    line(b, base - right * wid, c);
    line(b, base + up * wid, c);
    line(b, base - up * wid, c);
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

    glm::vec3 v[8];
    int i = 0;
    for (int z = -1; z <= 1; z += 2) {
      for (int y = -1; y <= 1; y += 2) {
        for (int x = -1; x <= 1; x += 2) {
          v[i++] = glm::vec3(xform * glm::vec4(0.5f * x, 0.5f * y, 0.5f * z, 1.0f));
        }
      }
    }

    constexpr int e[12][2] = {
      { 0, 1 },
      { 2, 3 },
      { 4, 5 },
      { 6, 7 },
      { 0, 2 },
      { 1, 3 },
      { 4, 6 },
      { 5, 7 },
      { 0, 4 },
      { 1, 5 },
      { 2, 6 },
      { 3, 7 },
    };
    for (auto& pr : e) {
      line(v[pr[0]], v[pr[1]], c);
    }
  }

  void debug_draw::sphere(const glm::vec3& center, float r, const glm::vec4& c, int rings) {
    if (sink == nullptr) {
      return;
    }

    const int seg = glm::max(rings, 4);
    const float step = glm::two_pi<float>() / float(seg);
    for (int i = 0; i < seg; ++i) {
      const float t0 = i * step;
      const float t1 = (i + 1) * step;
      const glm::vec2 a{ glm::cos(t0), glm::sin(t0) }, b{ glm::cos(t1), glm::sin(t1) };

      line(center + r * glm::vec3(a.x, a.y, 0), center + r * glm::vec3(b.x, b.y, 0), c);
      line(center + r * glm::vec3(a.x, 0, a.y), center + r * glm::vec3(b.x, 0, b.y), c);
      line(center + r * glm::vec3(0, a.x, a.y), center + r * glm::vec3(0, b.x, b.y), c);
    }
  }

  void debug_draw::frustum(const glm::mat4& inv_view_proj, const glm::vec4& c) {
    if (sink == nullptr) {
      return;
    }

    const glm::vec3 ndc[8] = {
      { -1, -1, -1 },
      { 1, -1, -1 },
      { 1, 1, -1 },
      { -1, 1, -1 },
      { -1, -1, 1 },
      { 1, -1, 1 },
      { 1, 1, 1 },
      { -1, 1, 1 },
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

    glm::vec3 w[8];
    for (int i = 0; i < 8; ++i) {
      glm::vec4 p = inv_view_proj * glm::vec4(ndc[i], 1.0f);
      w[i] = glm::vec3(p) / p.w;
    }

    for (auto& pr : e) {
      line(w[pr[0]], w[pr[1]], c);
    }
  }

  void debug_draw::transform(const glm::mat4& m, float scale) {
    if (sink == nullptr) {
      return;
    }

    const glm::vec3 o = glm::vec3(m[3]);
    line(o, o + scale * glm::vec3(m[0]), basic_colors::kRed);
    line(o, o + scale * glm::vec3(m[1]), basic_colors::kGreen);
    line(o, o + scale * glm::vec3(m[2]), basic_colors::kBlue);
  }

  void debug_draw::mesh(resource_handle h, const glm::mat4& model, const glm::vec4& tint, bool wireframe) {
    if (sink == nullptr) {
      return;
    }
    sink->submit(builtin_debug_streams::kMeshes, debug_mesh_instance{
                                                   .mesh_handle = h,
                                                   .model = model,
                                                   .color = tint,
                                                   .submesh_index = 0,
                                                   .flags = static_cast<uint32_t>(wireframe ? debug_mesh_instance::DEBUG_MESH_WIREFRAME : debug_mesh_instance::DEBUG_MESH_NONE),
                                                 });
  }

}  // namespace other