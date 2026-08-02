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

  /// a stream-name set a debug_draw writes into, letting the same emitter interface target
  /// either the debug overlay view or the in-scene (depth tested) overlay pass
  struct draw_stream_set {
    std::string_view lines;
    std::string_view tris;
    std::string_view points;
    std::string_view meshes;
  };

  /// drawn on top of the frame by the editor debug-overlay pipeline
  namespace builtin_debug_streams {

    constexpr std::string_view kLines = "__debug.lines";
    constexpr std::string_view kTris = "__debug.tris";
    constexpr std::string_view kPoints = "__debug.points";
    constexpr std::string_view kMeshes = "__debug.meshes";

    constexpr draw_stream_set kSet{ kLines, kTris, kPoints, kMeshes };

  }  // namespace builtin_debug_streams

  /// drawn inside the scene pipeline, depth tested against scene geometry
  namespace builtin_scene_streams {

    constexpr std::string_view kLines = "__scene.lines";
    constexpr std::string_view kTris = "__scene.tris";
    constexpr std::string_view kPoints = "__scene.points";

    /// meshes have no in-scene pass yet, in-scene mesh submissions fall back to the debug overlay
    constexpr draw_stream_set kSet{ kLines, kTris, kPoints, builtin_debug_streams::kMeshes };

  }  // namespace builtin_scene_streams

  class render_stream;

  struct debug_vertex {
    glm::vec3 position;
    glm::vec4 color;
  };

  /// one procedural grid submitted for this frame, drawn inside the scene pipeline by the
  ///   "scene_grids" pass executor (see renderer::submit_grid)
  struct grid_draw_data {
    glm::vec4 basis_u;  //< xyz = world-space unit axis spanning the plane
    glm::vec4 basis_v;
    glm::vec4 origin;  //< world-space grid origin
    glm::vec4 line_color;
    glm::vec4 major_line_color;
    glm::vec4 axis_u_color;
    glm::vec4 axis_v_color;
    float cell_size = 1.f;
    float line_width = 1.5f;  //< in pixels
    uint32_t extent = 0;      //< cells (cartesian) or rings (polar) from the origin
    uint32_t major_line_every = 0;
    uint32_t sector_count = 0;  //< polar radial spokes
    bool polar = false;  //< false = cartesian lattice, true = polar rings/spokes
    bool show_axes = false;
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
    debug_draw(render_stream* sink, const draw_stream_set& streams = builtin_debug_streams::kSet) : sink(sink), streams(streams) {}

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
    draw_stream_set streams;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_DEBUG_DRAW_HPP