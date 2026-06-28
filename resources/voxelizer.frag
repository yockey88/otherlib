#include "shader-modules/simulation-environment.glsl"
#include "shader-modules/basic-lighting.glsl"

layout(rgba16f) uniform writeonly image3D OE_voxel_tex; 
in vec3 f_world_pos;

void main() {
  ivec3 dim = imageSize(OE_voxel_tex);
  ivec3 c = oe_world_to_voxel(f_world_pos, dim);
  if (all(greaterThanEqual(c, ivec3(0))) && all(lessThan(c, dim))) {
    imageStore(OE_voxel_tex, c, vec4(1.0));
  }
}