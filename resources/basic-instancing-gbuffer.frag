#include "shader-modules/basic-material.glsl"

layout (location = 0) out vec4 OE_gbuff_albedo;
layout (location = 1) out vec4 OE_gbuff_normal;
layout (location = 2) out vec3 OE_gbuff_position;
layout (location = 3) out float OE_gbuff_depth;

in vec3 world_normal;
in vec3 world_position;

out vec4 frag_color;

void main() {
  material mat = get_instance_material();
  vec4 base_color = material_base_color();

  /// screen-door transparency: authored opacity is base_color.a (material * texture). the
  /// deferred gbuffer keeps one surface per pixel, so coverage is dithered against a 4x4 bayer
  /// threshold instead of blended; SMAA downstream softens the pattern. opaque materials
  /// (alpha >= 1) never discard. the shadow-map and voxelize passes bind no material buffer,
  /// so dithered surfaces still cast full shadows and occupy the AO volume (accepted POC limits).
  if (base_color.a < 1.0) {
    const float bayer[16] = float[16](
       0.0 / 16.0,  8.0 / 16.0,  2.0 / 16.0, 10.0 / 16.0,
      12.0 / 16.0,  4.0 / 16.0, 14.0 / 16.0,  6.0 / 16.0,
       3.0 / 16.0, 11.0 / 16.0,  1.0 / 16.0,  9.0 / 16.0,
      15.0 / 16.0,  7.0 / 16.0, 13.0 / 16.0,  5.0 / 16.0);
    ivec2 px = ivec2(gl_FragCoord.xy) & 3;
    if (base_color.a < bayer[px.y * 4 + px.x] + (0.5 / 16.0)) {
      discard;
    }
  }

  OE_gbuff_position = world_position;
  /// metalness rides normal.a the same way roughness rides albedo.a; every other reader of
  /// this target consumes .xyz only
  OE_gbuff_normal = vec4(normalize(world_normal), mat.metalness);
  OE_gbuff_albedo.rgb = base_color.rgb;
  /// roughness rides albedo.a into the shading pass (formerly specular_reflect; the pass
  /// already treats .a as a scalar shading factor)
  OE_gbuff_albedo.a = mat.roughness;
  OE_gbuff_depth = gl_FragCoord.z;
}