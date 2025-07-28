struct point_light {
  vec4 position;
  vec4 color;
};

layout (std430, binding = 0) readonly buffer point_light_buffer {
  point_light point_lights[];
};

uniform vec3 light_pos;
uniform float far_plane;

in vec4 frag_pos;

void main() {
  float distance = length(frag_pos.xyz - light_pos);
  distance = distance / far_plane;
  gl_FragDepth = distance;
}