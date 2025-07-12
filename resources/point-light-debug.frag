#include "shader-modules/basic-instancing-fragment.glsl"

in vec4 frag_color;

out vec4 color;

void main() {
  color = frag_color;
}