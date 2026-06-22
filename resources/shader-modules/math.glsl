const float epsilon = 0.0001f;
const float infinity = 1.0/0.0;
const float kPi = 3.14159265358979323846f;

struct interval {
  float min;
  float max;
};

struct intersection_record {
  bool hit;
  float t;
  int idx;

  bool front_face;
  vec3 normal;
};

struct ray {
  vec3 origin;
  vec3 direction;
};

bool interval_contains(interval i, float a) {
  return i.min <= a && a <= i.max;
}

bool interval_surronds(interval i, float a) {
  return i.min < a && a < i.max;
}

float interval_clamp(interval range, float t) {
  if (t < range.min) {
    return range.min;
  }
  if (t > range.max) {
    return range.max;
  }
  return t;
}

void interval_clamp_vec3(interval range, inout vec3 v) {
  v.x = interval_clamp(range, v.x);
  v.y = interval_clamp(range, v.y);
  v.z = interval_clamp(range, v.z);
}

vec3 get_ray_point(float t, ray r) {
  return r.origin + t * r.direction;
}

void check_face_orientation(inout intersection_record record, ray r, vec3 normal) { 
  record.front_face = dot(r.direction, normal) < 0.f;
  if (record.front_face) {
    record.normal = normal;
  } else {
    record.normal = -normal;
  }
}

vec3 reflect(vec3 v, vec3 n) {
  return v - 2 * dot(v, n) * n;
}

vec3 refract(vec3 uv, vec3 n, float etai_over_etat) {
  float cos_theta = min(dot(-uv, n), 1.0);
  
  vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
  float len_sq = length(r_out_perp) * length(r_out_perp);

  vec3 r_out_parallel = -sqrt(abs(1.0 - len_sq)) * n;
  return r_out_perp + r_out_parallel;
}

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