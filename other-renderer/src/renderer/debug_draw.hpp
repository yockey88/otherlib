/**
 * \file renderer/debug_draw.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_DEBUG_DRAW_HPP
#define OTHER_RENDERER_RENDERER_DEBUG_DRAW_HPP

#include <glm/glm.hpp>

#include "math/bounding_box.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/colors.hpp"

namespace other {

  namespace builtin_debug_streams {

    constexpr std::string_view kLines = "__debug.lines";
    constexpr std::string_view kTris = "__debug.tris";
    constexpr std::string_view kPoints = "__debug.points";
    constexpr std::string_view kMeshes = "__debug.meshes";

  }  // namespace builtin_debug_streams

  class render_stream;

  struct debug_vertex {
    glm::vec3 position;
    glm::vec4 color;
  };

  struct debug_mesh_instance {
    enum draw_flags {
      DEBUG_MESH_NONE = 0,
      DEBUG_MESH_WIREFRAME = 1 << 0,
      DEBUG_MESH_NO_DEPTH = 1 << 1,
    };
    resource_handle mesh_handle;
    glm::mat4 model;
    glm::vec4 color;
    uint32_t submesh_index = 0;
    uint32_t flags = 0;
  };

  class debug_draw {
   public:
    debug_draw(render_stream* sink) : sink(sink) {}

    void line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c = basic_colors::kWhite);
    void triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& d, const glm::vec4& c);
    void point(const glm::vec3& p, const glm::vec4& c = basic_colors::kWhite);

    void ray(const glm::vec3& o, const glm::vec3& dir, float len, const glm::vec4& c = basic_colors::kYellow);
    void arrow(const glm::vec3& a, const glm::vec3& b, const glm::vec4& c = basic_colors::kYellow);

    void aabb(const bounding_box& b, const glm::vec4& c = basic_colors::kGreen);
    void aabb(const glm::vec3& mn, const glm::vec3& mx, const glm::vec4& c = basic_colors::kGreen);

    void obb(const glm::mat4& xform, const glm::vec4& c = basic_colors::kGreen);
    void sphere(const glm::vec3& center, float r, const glm::vec4& c = basic_colors::kBlue, int rings = 16);
    void frustum(const glm::mat4& inv_view_proj, const glm::vec4& c = basic_colors::kWhite);
    void transform(const glm::mat4& m, float scale = 1.0f);  // R/G/B axes

    void mesh(resource_handle h, const glm::mat4& model, const glm::vec4& tint = basic_colors::kWhite, bool wireframe = false);

   private:
    render_stream* sink;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DEBUG_DRAW_HPP