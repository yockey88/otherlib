layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitanget;
layout (location = 4) in vec2 OE_tex_coords;

out vec3 world_position;
out vec3 world_normal;
out int OE_material_index;

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

uniform mat4 u_model_matrix;

mat4 get_camera_matrix() {
  return projection_matrix * view_matrix;
}

mat4 get_instance_model_matrix() {
  return u_model_matrix;
}

vec4 get_local_position() {
  return vec4(OE_position, 1.0);
}

vec4 get_world_position() {
  return get_instance_model_matrix() * get_local_position();
}

mat4 get_instance_mvp_matrix() {
  return get_camera_matrix() * get_instance_model_matrix();
}

vec4 get_normal() {
  return vec4(OE_normal, 1.0);
}
  
void main() {
  mat4 model_matrix = get_instance_model_matrix();
  world_position = get_world_position().xyz;

  mat3 normal_mat = transpose(inverse(mat3(model_matrix)));
  world_normal = normalize(model_matrix * get_normal()).xyz;

  gl_Position = get_instance_mvp_matrix() * vec4(OE_position, 1.0);
}