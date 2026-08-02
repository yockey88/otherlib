#include "shader-modules/camera.glsl"

layout(location = 0) in vec2 OE_position;
layout(location = 1) in vec2 OE_tex_coords;

uniform vec4 OE_grid_basis_u;
uniform vec4 OE_grid_basis_v;
uniform vec4 OE_grid_origin;
uniform float OE_grid_cell_size;
uniform int OE_grid_extent;

layout(location = 0) out vec2 OE_plane_coords;

void main() {
  /// stretch the pipeline quad over the grid plane in world space
  float radius = float(OE_grid_extent) * OE_grid_cell_size;
  OE_plane_coords = OE_position * radius;
  vec3 world_position = OE_grid_origin.xyz + OE_grid_basis_u.xyz * OE_plane_coords.x + OE_grid_basis_v.xyz * OE_plane_coords.y;
  gl_Position = get_camera_matrix() * vec4(world_position, 1.0);
}
