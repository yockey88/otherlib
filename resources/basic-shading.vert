#include "shader-modules/basic-deferred-shading-vertex.glsl"

out vec2 frag_tex_coords;

void main() {
  set_mat_idx();
  frag_tex_coords = get_texture_coords();
  gl_Position = get_position();
}