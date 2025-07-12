#include "shader-modules/basic-instancing-fragment.glsl"

in vec3 world_normal;
in vec3 world_position;

out vec4 frag_color;

void main() {
  material mat = get_instance_material();
  
  vec3 color = vec3(0);
  color += calculate_point_light_color(mat, world_normal, world_position);
  color += calculate_direction_light_color(mat, world_normal);  
  
  vec3 spec = mat.specular_reflect * mat.specular_color;
  vec3 diff = mat.diffuse_reflect * mat.diffuse_color;
  color = (diff + spec) * color + clamp(mat.emissivity, 0, 1) * mat.diffuse_color;

  frag_color = vec4(color, 1.0); 
}