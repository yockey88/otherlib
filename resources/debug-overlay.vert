#include "shader-modules/camera.glsl"

layout(location = 0) in vec3 OE_position;
layout(location = 1) in vec4 OE_color;

layout(location = 0) out vec4 OE_vert_color;

void main() {
  OE_vert_color = OE_color;
  gl_Position = get_camera_matrix() * vec4(OE_position, 1.0);
  gl_PointSize = 4.0;
}