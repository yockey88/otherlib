float random_float(inout uint seed) {
  seed = seed * 747796405u + 2891336453u;
  uint result = ((seed >> ((seed >> 28u) + 4u)) ^ seed) * 277803737u;
  result = (result >> 22u) ^ result;
  return float(result) / 4294967295.0;
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
  vec3 p;
  do {
    p = random_vec3(-1.0, 1.0, seed);
    float lensq = dot(p, p);
    if (epsilon < lensq && lensq <= 1.0) {
      return normalize(p);
    }
  } while (true);
  
  return p; // vec3(x, y, z);
}

vec3 random_cosine_hemisphere(vec3 normal, inout uint seed) {
  float r1 = random_float(seed);
  float r2 = random_float(seed);

  float cos_theta = sqrt(r1);
  float sin_theta = sqrt(1.0 - r1);
  float phi = 2.0 * kPi * r2;

  float x = sin_theta * cos(phi);
  float y = sin_theta * sin(phi);
  float z = cos_theta;
  
  vec3 w = normalize(normal);
  vec3 u = normalize(cross((abs(w.x) > 0.1) ? vec3(0, 1, 0) : vec3(1, 0, 0), w));
  vec3 v = cross(w, u);
  
  return x * u + y * v + z * w;
}