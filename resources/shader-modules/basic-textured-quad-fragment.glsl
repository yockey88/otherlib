uniform sampler2D OE_texture;

vec3 get_color(vec2 coords) {
  return texture(OE_texture, coords).rgb;
}

uniform float OE_exposure;
uniform float OE_gamma;