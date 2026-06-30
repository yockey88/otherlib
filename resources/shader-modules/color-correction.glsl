vec3 oe_srgb_to_linear(vec3 c) {
  bvec3 cutoff = lessThanEqual(c, vec3(0.04045));
  vec3  lo = c / 12.92;
  vec3  hi = pow((c + 0.055) / 1.055, vec3(2.4));
  return mix(hi, lo, cutoff);
}

vec3 oe_linear_to_srgb(vec3 c) {
  bvec3 cutoff = lessThanEqual(c, vec3(0.0031308));
  vec3  lo = c * 12.92;
  vec3  hi = 1.055 * pow(max(c, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
  return mix(hi, lo, cutoff);
}

vec3 oe_gamma_to_linear(vec3 c, float gamma) { 
  return pow(max(c, vec3(0.0)), vec3(gamma)); 
}

vec3 oe_linear_to_gamma(vec3 c, float gamma) { 
  return pow(max(c, vec3(0.0)), vec3(1.0 / gamma)); 
}

vec3 oe_tonemap_reinhard(vec3 x) { 
  return x / (1.0 + x); 
}

vec3 oe_tonemap_exposure(vec3 x, float ex)   { 
  return vec3(1.0) - exp(-x * ex); 
}

// ACES filmic fit (Narkowicz 2015)
// linear -> linear [0,1].
vec3 oe_tonemap_aces(vec3 x) {
  const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}