#include "shader-modules/basic-textured-quad-fragment.glsl"

in vec2 frag_tex_coords;

out vec4 frag_color;

#if 1
#define SRGB
#endif

void main() {
#ifdef SRGB
  float gamma = 2.2;
  frag_color.rgb = pow(get_color(frag_tex_coords), vec3(1.0/gamma));
#else
  frag_color.rgb = get_color(frag_tex_coords);
#endif 
}