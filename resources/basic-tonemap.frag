#include "shader-modules/basic-textured-quad-fragment.glsl"

in vec2 frag_tex_coords;
out vec4 frag_color;

void main() {
  vec3 hdr_color = get_color(frag_tex_coords);
  vec3 mapped = vec3(1.0) - exp(-hdr_color * OE_exposure);
  frag_color = vec4(pow(hdr_color, vec3(1.0 / OE_gamma)), 1.0);
}
