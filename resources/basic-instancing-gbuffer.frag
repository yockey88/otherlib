#include "shader-modules/basic-material.glsl"

layout (location = 0) out vec4 OE_gbuff_albedo;
layout (location = 1) out vec3 OE_gbuff_normal;
layout (location = 2) out vec3 OE_gbuff_position;
layout (location = 3) out float OE_gbuff_depth;

in vec3 world_normal;
in vec3 world_position;

out vec4 frag_color;

void main() {
  material mat = get_instance_material();

  OE_gbuff_position = world_position;
  OE_gbuff_normal = normalize(world_normal);
  OE_gbuff_albedo.rgb = material_base_color().rgb;
  /// roughness rides albedo.a into the shading pass (formerly specular_reflect; the pass
  /// already treats .a as a scalar shading factor)
  OE_gbuff_albedo.a = mat.roughness;
  OE_gbuff_depth = gl_FragCoord.z;
}