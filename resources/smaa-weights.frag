uniform sampler2D OE_edges;    // slot 0
uniform sampler2D OE_area;     // slot 1 — AreaTex   (160x560 RG8)
uniform sampler2D OE_search;   // slot 2 — SearchTex (64x16  R8)

#define SMAA_RT_METRICS vec4(1.0 / vec2(textureSize(OE_edges, 0)), vec2(textureSize(OE_edges, 0)))
#define SMAA_GLSL_4
#define SMAA_PRESET_HIGH
#define SMAA_INCLUDE_VS 0
#include "shader-modules/smaa.glsl"

in vec2 frag_tex_coords;
in vec2 v_pixcoord;
in vec4 v_offset[3];
out vec4 frag_color;

void main() {
  frag_color = SMAABlendingWeightCalculationPS(frag_tex_coords, v_pixcoord, v_offset, OE_edges, OE_area, OE_search, vec4(0.0));
}