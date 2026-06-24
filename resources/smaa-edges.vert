#include "shader-modules/basic-textured-quad-vertex.glsl"

uniform sampler2D OE_color;

#define SMAA_RT_METRICS vec4(1.0 / vec2(textureSize(OE_color, 0)), vec2(textureSize(OE_color, 0)))
#define SMAA_GLSL_4
#define SMAA_PRESET_HIGH
#define SMAA_INCLUDE_PS 0
#include "shader-modules/smaa.glsl"

out vec2 frag_tex_coords;
out vec4 v_offset[3];

void main() {
  vec2 uv = get_texture_coords();
  frag_tex_coords = uv;
  SMAAEdgeDetectionVS(uv, v_offset);
  gl_Position = get_position();
}
