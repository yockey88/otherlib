uniform sampler2D scene_tex;

in vec2 frag_tex_coords;
out vec4 frag_color;

void main() {
  frag_color = texture(OE_texture, frag_tex_coords);
}