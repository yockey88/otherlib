layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitanget;
layout (location = 4) in vec2 OE_tex_coords;
layout (location = 5) in ivec4 OE_bone_ids;
layout (location = 6) in vec4 OE_bone_weights;

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

uniform mat4 OE_model_matrix;

mat4 get_mvp_matrix() {
  return projection_matrix * view_matrix * OE_model_matrix;
}

vec4 get_local_position() {
  return vec4(OE_position, 1.0);
}