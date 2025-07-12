uint hash(uint x) {
  x += (x << 10u);
  x ^= (x >> 6u);
  x += (x << 3u);
  x ^= (x >> 11u);
  x += (x << 15u);
  return x;
}

uint hash(uint x, uint y) {
  return hash(x ^ hash(y));
}

uint hash(uint x, uint y, uint z) {
  return hash(x ^ hash(y) ^ hash(z << 1u));
}

float random_float(inout uint seed) {
  seed = hash(seed);
  return float(seed) / float(0xffffffffu);
}

vec2 random_square(inout uint seed) {
  return vec2(random_float(seed), random_float(seed));
}

vec3 random_vec3(float min, float max, inout uint seed) {
  return vec3(
    min + (max - min) * random_float(seed),
    min + (max - min) * random_float(seed),
    min + (max - min) * random_float(seed)
  );
}

vec3 random_unit_vector(inout uint seed) {
  return normalize(random_vec3(-1.0, 1.0, seed));
}

vec3 random_cosine_hemisphere(vec3 normal, inout uint seed) {
  float r1 = random_float(seed);
  float r2 = random_float(seed);

  float cos_theta = sqrt(r1);
  float sin_theta = sqrt(1.0 - r1);
  float phi = 2.0 * kPi * r2;

  vec3 w = normal;
  vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0, 1, 0) : vec3(1, 0, 0), w));
  vec3 v = cross(w, u);
  
  return normalize(u * cos(phi) * sin_theta + v * sin(phi) * sin_theta + w * cos_theta);
}