uniform sampler2D OE_color;

#define SMAA_RT_METRICS vec4(1.0 / vec2(textureSize(OE_color, 0)), vec2(textureSize(OE_color, 0)))
#define SMAA_GLSL_4
#define SMAA_PRESET_HIGH
#define SMAA_INCLUDE_VS 0
#include "shader-modules/smaa.glsl"

in vec2 frag_tex_coords;
in vec4 v_offset[3];
out vec4 frag_color;

void main() {
  // Use SMAAColorEdgeDetectionPS instead for chroma-only edges (slightly costlier).
  vec2 edges = SMAALumaEdgeDetectionPS(frag_tex_coords, v_offset, OE_color);
  frag_color = vec4(edges, 0.0, 0.0);   // only .rg are stored in the RG8 target
}