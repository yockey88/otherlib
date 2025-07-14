#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 world_position;
out vec3 world_normal;

void main() {
  set_instance_id();

  mat4 model_mat = get_instance_model_matrix();
  vec3 world_position = get_world_position().xyz;

  mat3 normal_mat = transpose(inverse(mat3(model_mat)));
  world_normal = (model_mat * get_normal()).xyz;

  gl_Position = get_camera_matrix() * get_world_position();
}