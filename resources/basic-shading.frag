#include "shader-modules/basic-deferred-shading-fragment.glsl"

in vec2 frag_tex_coords;

out vec4 frag_color;

void main() {     
  vec4 albedo = texture(OE_gbuff_albedo, frag_tex_coords);        
  vec3 diffuse = albedo.rgb;
  float specular_reflect = albedo.a;

  vec3 normal = texture(OE_gbuff_normal, frag_tex_coords).rgb;
  vec3 position = texture(OE_gbuff_position, frag_tex_coords).rgb;
  
  frag_color = calculate_lighting(diffuse, position, normal, specular_reflect);
}