#include "shader-modules/math.glsl"

layout(location = 0) in vec2 OE_plane_coords;

uniform vec4 OE_grid_line_color;
uniform vec4 OE_grid_major_line_color;
uniform vec4 OE_grid_axis_u_color;
uniform vec4 OE_grid_axis_v_color;
uniform float OE_grid_cell_size;
uniform float OE_grid_line_width;
uniform int OE_grid_extent;
uniform int OE_grid_major_every;
uniform int OE_grid_sector_count;
uniform int OE_grid_polar;
uniform int OE_grid_rings_only;
uniform int OE_grid_show_axes;

out vec4 frag_color;

/// anti-aliased mask for the periodic lines at integer values of t, ft = pixel footprint of t
float oe_grid_line_mask(float t, float ft, float width_px) {
  float half_width = 0.5 * width_px * ft;
  float dist = abs(fract(t + 0.5) - 0.5);
  return 1.0 - smoothstep(half_width, half_width + ft, dist);
}

/// anti-aliased mask for the single line at t == 0
float oe_grid_axis_mask(float t, float ft, float width_px) {
  float half_width = 0.5 * width_px * ft;
  return 1.0 - smoothstep(half_width, half_width + ft, abs(t));
}

/// fades a line family out before its spacing drops under ~2 pixels and starts to moire
float oe_grid_lod_fade(float ft) {
  return 1.0 - smoothstep(0.25, 0.5, ft);
}

/// standard premultiplied-style "over" compositing of straight-alpha layers
vec4 oe_grid_layer_over(vec4 below, vec4 layer_color, float mask) {
  vec4 above = vec4(layer_color.rgb, layer_color.a * mask);
  float alpha = above.a + below.a * (1.0 - above.a);
  if (alpha <= 0.0001) {
    return vec4(0.0);
  }
  vec3 rgb = (above.rgb * above.a + below.rgb * below.a * (1.0 - above.a)) / alpha;
  return vec4(rgb, alpha);
}

vec4 oe_cartesian_grid() {
  vec2 cells = OE_plane_coords / OE_grid_cell_size;
  vec2 ft = fwidth(cells);
  float width = OE_grid_line_width;

  float minor = max(
    oe_grid_line_mask(cells.x, ft.x, width) * oe_grid_lod_fade(ft.x),
    oe_grid_line_mask(cells.y, ft.y, width) * oe_grid_lod_fade(ft.y));

  float major = 0.0;
  if (OE_grid_major_every > 0) {
    vec2 major_cells = cells / float(OE_grid_major_every);
    vec2 major_ft = ft / float(OE_grid_major_every);
    major = max(
      oe_grid_line_mask(major_cells.x, major_ft.x, width) * oe_grid_lod_fade(major_ft.x),
      oe_grid_line_mask(major_cells.y, major_ft.y, width) * oe_grid_lod_fade(major_ft.y));
  }

  vec4 color = oe_grid_layer_over(vec4(0.0), OE_grid_line_color, minor);
  color = oe_grid_layer_over(color, OE_grid_major_line_color, major);

  if (OE_grid_show_axes != 0) {
    /// the u axis runs along v == 0 and vice versa
    color = oe_grid_layer_over(color, OE_grid_axis_u_color, oe_grid_axis_mask(cells.y, ft.y, width));
    color = oe_grid_layer_over(color, OE_grid_axis_v_color, oe_grid_axis_mask(cells.x, ft.x, width));
  }

  /// soft square falloff toward the grid extent
  float boundary = max(abs(cells.x), abs(cells.y)) / float(OE_grid_extent);
  color.a *= 1.0 - smoothstep(0.85, 1.0, boundary);
  return color;
}

vec4 oe_polar_grid() {
  float width = OE_grid_line_width;
  float radius_world = length(OE_plane_coords);
  /// approximate world units per pixel at this fragment, shared by all polar footprints
  vec2 fw_plane = fwidth(OE_plane_coords);
  float pixel_world = max(max(fw_plane.x, fw_plane.y), 0.0000001);

  /// stacked cylindrical layers keep only their ring families: majors when configured, minors otherwise
  bool rings_only = OE_grid_rings_only != 0;

  /// rings at integer multiples of the cell size
  float rings_t = radius_world / OE_grid_cell_size;
  float rings_ft = pixel_world / OE_grid_cell_size;
  float rings = oe_grid_line_mask(rings_t, rings_ft, width) * oe_grid_lod_fade(rings_ft);
  if (rings_only && OE_grid_major_every > 0) {
    rings = 0.0;
  }

  float major = 0.0;
  if (OE_grid_major_every > 0) {
    float major_t = rings_t / float(OE_grid_major_every);
    float major_ft = rings_ft / float(OE_grid_major_every);
    major = oe_grid_line_mask(major_t, major_ft, width) * oe_grid_lod_fade(major_ft);
  }

  /// angular footprint derived analytically to stay stable across the atan seam
  float theta = atan(OE_plane_coords.y, OE_plane_coords.x);
  float theta_ft = pixel_world / max(radius_world, 0.0000001);
  /// spokes crowd into the center, fade them below the first ring
  float center_fade = smoothstep(0.5, 1.5, rings_t);

  float spokes = 0.0;
  if (OE_grid_sector_count > 0 && !rings_only) {
    float sector_t = theta / (2.0 * kPi) * float(OE_grid_sector_count);
    float sector_ft = theta_ft / (2.0 * kPi) * float(OE_grid_sector_count);
    spokes = oe_grid_line_mask(sector_t, sector_ft, width) * oe_grid_lod_fade(sector_ft) * center_fade;
  }

  vec4 color = oe_grid_layer_over(vec4(0.0), OE_grid_line_color, max(rings, spokes));
  color = oe_grid_layer_over(color, OE_grid_major_line_color, major);

  if (OE_grid_show_axes != 0) {
    /// the +u spoke doubles as the reference axis
    float axis_t = theta / (2.0 * kPi);
    float axis_ft = theta_ft / (2.0 * kPi);
    color = oe_grid_layer_over(color, OE_grid_axis_u_color, oe_grid_axis_mask(axis_t, axis_ft, width) * center_fade);
  }

  /// circular falloff toward the outer ring, also clips the square quad corners
  float boundary = rings_t / float(OE_grid_extent);
  color.a *= 1.0 - smoothstep(0.85, 1.0, boundary);
  return color;
}

void main() {
  vec4 color = OE_grid_polar != 0 ? oe_polar_grid() : oe_cartesian_grid();
  if (color.a <= 0.001) {
    discard;
  }
  frag_color = color;
}
