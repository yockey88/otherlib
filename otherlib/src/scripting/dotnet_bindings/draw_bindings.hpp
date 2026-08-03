/**
 * \file scripting/dotnet_bindings/draw_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_DRAW_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_DRAW_BINDINGS_HPP

#include <cstdint>

#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    /// dynamic draw submissions; in_scene selects the depth-tested in-scene overlay streams,
    ///   otherwise the debug overlay view (drawn on top of the frame, editor only)
    void native_draw_line(float ax, float ay, float az, float bx, float by, float bz, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_point(float px, float py, float pz, float r, float g, float b, float a, nbool32 in_scene);

    void native_draw_ray(float ox, float oy, float oz, float dx, float dy, float dz, float len, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_arrow(float ax, float ay, float az, float bx, float by, float bz, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_aabb(float min_x, float min_y, float min_z, float max_x, float max_y, float max_z, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_obb(float m00, float m01, float m02, float m03,
                         float m10, float m11, float m12, float m13,
                         float m20, float m21, float m22, float m23,
                         float m30, float m31, float m32, float m33,
                         float r, float g, float b, float a,
                         nbool32 in_scene);
    void native_draw_sphere(float cx, float cy, float cz, float radius, float r, float g, float b, float a, nbool32 in_scene);
    void native_draw_frustum(float m00, float m01, float m02, float m03,
                             float m10, float m11, float m12, float m13,
                             float m20, float m21, float m22, float m23,
                             float m30, float m31, float m32, float m33,
                             float r, float g, float b, float a,
                             nbool32 in_scene);
    void native_draw_transform(float m00, float m01, float m02, float m03,
                               float m10, float m11, float m12, float m13,
                               float m20, float m21, float m22, float m23,
                               float m30, float m31, float m32, float m33,
                               float scale, nbool32 in_scene);
    void native_draw_mesh(uint64_t mesh_handle, float m00, float m01, float m02, float m03,
                          float m10, float m11, float m12, float m13,
                          float m20, float m21, float m22, float m23,
                          float m30, float m31, float m32, float m33,
                          float r, float g, float b, float a,
                          nbool32 wireframe, nbool32 in_scene);

    /// draws the grid_component on @p object_id where the object is this frame; in-scene grids
    ///   render procedurally (spherical falls back to depth-tested lines), debug grids as overlay lines
    void native_draw_grid(uint64_t object_id, nbool32 in_scene);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_DRAW_BINDINGS_HPP
