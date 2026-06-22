#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 g_world_pos;

void main() {
  set_instance_id();
  vec4 wp = get_world_position();   

  g_world_pos = wp.xyz;
  gl_Position = wp;
}