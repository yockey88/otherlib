#include "shader-modules/basic-instancing-vertex.glsl"

void main() {
  set_instance_id();
  gl_Position = get_instance_mvp_matrix() * get_local_position();
}