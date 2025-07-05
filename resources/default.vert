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

#ifdef WITH_GEOMETRY
out vec3 world_pos;
out vec3 world_normal;
#else
out vec3 frag_position;
out vec3 frag_normal;
#endif

void main() {
  // mat4 model_matrix = models[gl_InstanceID];

#ifdef WITH_GEOMETRY
  world_pos = (model_matrix * vec4(position, 1.0)).xyz;
  world_normal = (model_matrix * vec4(normal, 1.0)).xyz;
  gl_Position = projection_matrix * view_matrix * vec4(world_pos, 1.0);
#else
  frag_position = (model_matrix * vec4(position, 1.0)).xyz;
  frag_normal = (model_matrix * vec4(normal, 1.0)).xyz;
  gl_Position = projection_matrix * view_matrix * vec4(frag_position, 1.0);
#endif
}