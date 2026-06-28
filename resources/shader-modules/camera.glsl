#include "shader-modules/math.glsl"

layout (std140) uniform camera_buffer {
  vec4 camera_position;
  vec4 camera_forward;

  /// near & far clip, defocus_angle padding x2
  vec4 camera_features;

  vec4 defocus_disk_u;
  vec4 defocus_disk_v;

  mat4 view_matrix;
  mat4 projection_matrix;
};

mat4 get_camera_matrix() {
  return projection_matrix * view_matrix;
}

float camera_near_clip() { return camera_features.x; }
float camera_far_clip() { return camera_features.y; }
float camera_defocus_angle() { return camera_features.z; }

vec2 screen_to_ndc(ivec2 screen_coords, int screen_width, int screen_height) {
  float ndc_x = 2.0f * (screen_coords.x / float(screen_width)) - 1.0f;
  float ndc_y = 1.0f - 2.0f * (screen_coords.y / float(screen_height));
  return vec2(ndc_x, ndc_y);
}

vec4 ndc_to_view(vec2 ndc_coords) {
  vec4 homo = vec4(ndc_coords.x, ndc_coords.y, 0.f, 1.f);
  mat4 inv_projection = inverse(projection_matrix);
  vec4 view_coords = inv_projection * homo;

  if (view_coords.w != 0) {
    view_coords = view_coords / view_coords.w;
  }
  return view_coords;
}

vec3 view_to_world(vec4 view_coords) {
  vec4 homo = vec4(view_coords.xyz, 1);
  mat4 inv_view = inverse(view_matrix);
  vec4 world_coords = inv_view * homo;
  return world_coords.xyz;
}

vec3 sample_defocus_disk(inout uint seed) {
  vec3 ruv = random_unit_vector(seed);
  return camera_position.xyz + (ruv.x * defocus_disk_u.xyz) + (ruv.y * defocus_disk_v.xyz);
}

vec3 oe_view_ray(vec2 tex_coords) {
  vec2 ndc = tex_coords * 2.0 - 1.0;
  vec4 view_h = inverse(projection_matrix) * vec4(ndc, 1.0, 1.0);
  vec3 view_dir = normalize(view_h.xyz / view_h.w);
  vec3 world_dir = mat3(inverse(view_matrix)) * view_dir;
  return normalize(world_dir);
}