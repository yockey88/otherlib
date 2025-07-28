#include "shader-modules/basic-textured-quad-fragment.glsl"

in vec2 frag_tex_coords;

out vec4 frag_color;

#if 1
#define SRGB
#endif

void main() {
#ifdef SRGB
  float gamma = 2.2;
  vec3 hdr_color = get_color(frag_tex_coords);
  
  // reinhard
  // vec3 result = hdr_color / (hdr_color + vec3(1.0));

  // exposure
  // also gamma correct while we're at it       
  vec3 result = vec3(1.0) - exp(-hdr_color * OE_exposure);
  frag_color = vec4(pow(result, vec3(1.0 / gamma)), 1.0);

  // frag_color.rgb = pow(get_color(frag_tex_coords), vec3(1.0/gamma));
#else
  frag_color.rgb = get_color(frag_tex_coords);
#endif 
}