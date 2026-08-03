#include "shader-modules/camera.glsl"
#include "shader-modules/basic-material.glsl"
#include "shader-modules/simulation-environment.glsl"
#include "shader-modules/basic-lighting.glsl"
#include "shader-modules/scene-lighting.glsl"

in vec3 world_normal;
in vec3 world_position;

out vec4 frag_color;

/// forward-lit blended transparency: the same calculate_lighting as the deferred resolve, run
/// per fragment straight from the material, with authored opacity (material x texture x tint,
/// already folded into the packed base_color) carried out for src-alpha-over compositing
void main() {
  material mat = get_instance_material();
  vec4 base = material_base_color();

  vec4 lit = calculate_lighting(base.rgb, world_position, normalize(world_normal), mat.roughness, mat.metalness);
  frag_color = vec4(lit.rgb, base.a);
}
