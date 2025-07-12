#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 world_position;
out vec3 world_normal;

void main() {
  set_instance_id();
  mat4 model_matrix = get_instance_model_matrix();

  world_position = (model_matrix * get_local_position()).xyz;
  world_normal = (model_matrix * get_normal()).xyz;
  gl_Position = get_camera_matrix() * vec4(world_position, 1.0);
}