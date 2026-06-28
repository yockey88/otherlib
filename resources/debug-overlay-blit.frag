uniform sampler2D OE_frame;
uniform sampler2D OE_depth;

in vec2 frag_tex_coords;

out vec4 frag_color;

void main() {
  frag_color = texture(OE_frame, frag_tex_coords);
  gl_FragDepth = texture(OE_depth, frag_tex_coords).r;
}