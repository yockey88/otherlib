#include "shader-modules/basic-textured-quad-vertex.glsl"

out vec2 frag_tex_coords;

void main() {
  frag_tex_coords = get_texture_coords();
  gl_Position = get_position();
}