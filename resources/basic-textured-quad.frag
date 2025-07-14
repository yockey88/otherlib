#include "shader-modules/basic-textured-quad-fragment.glsl"

in vec2 frag_tex_coords;

out vec4 frag_color;

void main() {
  frag_color = vec4(get_color(frag_tex_coords), 1.0);
}