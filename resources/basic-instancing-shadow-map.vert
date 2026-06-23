#include "shader-modules/basic-geometry.glsl"
#include "shader-modules/basic-lighting.glsl"

void main() {
  gl_Position = OE_light_space_matrix * models[gl_InstanceID] * vec4(OE_position, 1.0);
}