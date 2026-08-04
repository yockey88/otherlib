#include "shader-modules/basic-geometry.glsl"

out vec3 g_world_pos;

void main() {
  set_instance_id();
  vec4 wp = get_instance_model_matrix() * get_bone_transform() * get_local_position();

  g_world_pos = wp.xyz;
  gl_Position = wp;
}