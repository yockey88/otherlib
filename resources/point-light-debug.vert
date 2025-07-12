#include "shader-modules/basic-instancing-vertex.glsl"

out vec4 frag_color;

void main() {
  frag_color = vec4(1.0);
  gl_Position = get_instance_mvp_matrix() * get_local_position();
}