#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 world_position;
out vec3 world_normal;

void main() {
  set_instance_id();

  mat4 model_matrix = get_instance_model_matrix();
  world_position = get_world_position().xyz;

  mat3 normal_mat = transpose(inverse(mat3(model_matrix)));
  world_normal = (model_matrix * get_normal()).xyz;

  gl_Position = get_camera_matrix() * vec4(world_position, 1.0);
}