layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vec3 world_pos[];
in vec3 world_normal[];

out vec3 frag_position;
out vec3 frag_normal;

void main() {
  const vec3 p1 = world_pos[1] - world_pos[0];
  const vec3 p2 = world_pos[2] - world_pos[0];
  const vec3 p = abs(cross(p1, p2));

  for (uint i = 0; i < 3; ++i) {
    frag_position = world_pos[i];
    frag_normal = world_normal[i];
		
    if(p.z > p.x && p.z > p.y){
			gl_Position = vec4(frag_position.x, frag_position.y, 0, 1);
		} else if (p.x > p.y && p.x > p.z){
			gl_Position = vec4(frag_position.y, frag_position.z, 0, 1);
		} else {
			gl_Position = vec4(frag_position.x, frag_position.z, 0, 1);
		}

    EmitVertex();
  }
  EndPrimitive();
}