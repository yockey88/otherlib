#include "shader-modules/simulation-environment.glsl"

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in  vec3 g_world_pos[];
out vec3 f_world_pos;

void main() {
  // world-space face normal 
  vec3 n = abs(cross(g_world_pos[1] - g_world_pos[0], g_world_pos[2] - g_world_pos[0]));
  int axis = (n.x >= n.y && n.x >= n.z) ? 
      0 : (n.y >= n.z ? 1 : 2);

  for (int i = 0; i < 3; ++i) {
    f_world_pos = g_world_pos[i];
    vec3 uvw = oe_world_to_uvw(g_world_pos[i]);
    vec2 p = (axis == 0) ? uvw.yz : (axis == 1) ? 
        uvw.xz : uvw.xy;
    
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
    EmitVertex();
  }
  
  EndPrimitive();
}