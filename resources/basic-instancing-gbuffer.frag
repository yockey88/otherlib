#include "shader-modules/basic-material.glsl"

layout (location = 0) out vec4 OE_gbuff_albedo;
layout (location = 1) out vec3 OE_gbuff_normal;
layout (location = 2) out vec3 OE_gbuff_position;

in vec3 world_normal;
in vec3 world_position;

out vec4 frag_color;

void main() {
  material mat = get_instance_material();
  
  OE_gbuff_position = world_position;
  OE_gbuff_normal = normalize(world_normal);
  OE_gbuff_albedo.rgb = mat.diffuse_color;
  OE_gbuff_albedo.a = mat.specular_reflect;
}