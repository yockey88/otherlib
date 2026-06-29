#include "shader-modules/basic-geometry.glsl"

void main() {
  gl_Position = get_camera_matrix() * get_world_position();
}