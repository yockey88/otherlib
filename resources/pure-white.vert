layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 bitanget;
layout (location = 4) in vec2 tex_coords;

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

uniform mat4 model_matrix;

out vec4 frag_color;

void main() {
  gl_Position = projection_matrix * view_matrix * model_matrix * vec4(position, 1.0);
  frag_color = vec4(1.0);
}