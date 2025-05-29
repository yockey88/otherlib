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

vec3 get_gradient(float a, vec3 col1, vec3 col2) {
  return (1.0 - a) * col1 + a * col2;
}

vec3 get_ray_point(float t, ray r) {
  return r.origin + t * r.direction;
}

#include "random.glsl"