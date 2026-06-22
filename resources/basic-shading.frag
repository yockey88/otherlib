#include "shader-modules/basic-deferred-shading-fragment.glsl"

in vec2 frag_tex_coords;

out vec4 frag_color;

void main() {
  vec4 albedo = texture(OE_gbuff_albedo, frag_tex_coords);        
  vec3 normal = texture(OE_gbuff_normal, frag_tex_coords).rgb;
  vec3 position = texture(OE_gbuff_position, frag_tex_coords).rgb;

  if (dot(normal, normal) < 0.5) {
    vec3 dir  = oe_view_ray(frag_tex_coords);
    frag_color = vec4(oe_sky_radiance(dir) * world_max.w, 1.0);
  } else {
    vec3 diffuse = albedo.rgb;
    float specular_reflect = albedo.a;
    frag_color = calculate_lighting(diffuse, position, normal, specular_reflect);
  }

}