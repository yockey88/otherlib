#include "shader-modules/basic-textured-quad-fragment.glsl"
#include "shader-modules/color-correction.glsl"

in vec2 frag_tex_coords;
out vec4 frag_color;

void main() {
  vec3 hdr     = get_color(frag_tex_coords);
  vec3 exposed = hdr * OE_exposure;
  vec3 mapped  = oe_tonemap_aces(exposed);
  vec3 display = oe_linear_to_gamma(mapped, OE_gamma);
  frag_color   = vec4(display, 1.0);
}
