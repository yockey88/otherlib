#include "shader-modules/camera.glsl"

layout(location = 0) in vec3 OE_position;   // model-space position attribute of the mesh

uniform mat4 OE_model;
uniform vec4 OE_tint;

layout(location = 0) out vec4 OE_vert_color;

void main() {
  OE_vert_color = OE_tint;
  gl_Position = get_camera_matrix() * OE_model * vec4(OE_position, 1.0);
}