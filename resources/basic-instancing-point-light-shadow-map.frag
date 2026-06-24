#include "shader-modules/basic-lighting.glsl"

uniform vec3 light_pos;
uniform float far_plane;

in vec4 frag_pos;

void main() {
  float distance = length(frag_pos.xyz - light_pos);
  distance = distance / far_plane;
  gl_FragDepth = distance;
}