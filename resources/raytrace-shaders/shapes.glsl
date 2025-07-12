struct sphere {
  vec3 position;
  float radius;
};

vec3 get_sphere_normal(sphere s, vec3 point) {
  return (point - s.position) / s.radius;
}

void intersect_sphere(sphere s, ray r, interval range, inout intersection_record record) {
  vec3 oc = r.origin - s.position;
  float a = dot(r.direction, r.direction);
  float h = -dot(r.direction, oc);
  float c = dot(oc, oc) - s.radius * s.radius;

  float discriminant = h * h - a * c;
  if (discriminant < 0) {
    record.hit = false;
    return;
  }

  float rt_dis = sqrt(discriminant);
  float t = (h - rt_dis) / a;
  if (!interval_contains(range, t)) {
    t = (h + rt_dis) / a;
    if (!interval_contains(range, t)) {
      record.hit = false;
      return;
    }
  }

  record.hit = true;
  record.t = t;
  record.normal = get_sphere_normal(s, get_ray_point(t, r));
}