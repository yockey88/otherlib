layout (location = 0) in vec2 OE_position;
layout (location = 1) in vec2 OE_tex_coords;

vec4 get_position() {
  return vec4(OE_position, 0.0, 1.0);
}

vec2 get_texture_coords() {
  return OE_tex_coords;
}