struct material {
  int material_type;
  int material_idx;
};

struct lambertian {
  vec3 albedo;
};

struct metal {
  vec3 albedo;
  float fuzziness;
};

struct dielectric {
  vec3 albedo;
  float refraction_idx;
};

layout (std140) uniform material_buffer {
  material materials[MAX_MATERIALS];
};

layout (std140) uniform lambertian_buffer {
  lambertian lambertian_materials[MAX_LAMBERTIAN];
};

layout (std140) uniform metal_buffer {
  metal metal_materials[MAX_METAL];
};

layout (std140) uniform dielectric_buffer {
  dielectric dielectric_materials[MAX_DIELECTRIC];
};

lambertian material_as_lambertian(material mat) {
  if (mat.material_type != MATERIAL_LAMBERTIAN || mat.material_idx >= MAX_LAMBERTIAN) {
    return lambertian(vec3(0));
  }
  return lambertian_materials[mat.material_idx];
}

vec3 scatter_lambertian(lambertian lambert, inout ray r, intersection_record rec, inout uint seed) {
  vec3 dir = rec.normal + random_unit_vector(seed);
  r = ray(get_ray_point(rec.t, r), dir);
  return lambert.albedo;
}

metal material_as_metal(material mat) {
  if (mat.material_type != MATERIAL_METAL || mat.material_idx >= MAX_METAL) {
    return metal(vec3(0), 0.f);
  }
  return metal_materials[mat.material_idx];
}

vec3 scatter_metal(metal met, inout ray r, intersection_record rec, inout uint seed) {
  vec3 reflected = reflect(r.direction, rec.normal);
  reflected = normalize(reflected) + (met.fuzziness * random_unit_vector(seed));

  r = ray(get_ray_point(rec.t, r), reflected);
  return met.albedo;
}

dielectric material_as_dielectric(material mat) {
  if (mat.material_type != MATERIAL_DIELECTRIC || mat.material_idx >= MAX_DIELECTRIC) {
    return dielectric(vec3(0), 0.f);
  }
  return dielectric_materials[mat.material_idx];
}

float reflectance(float cosine, float refraction_index) {
  float r0 = (1 - refraction_index) / (1 + refraction_index);
  r0 = r0 * r0;
  return r0 + (1 - r0) * pow((1 - cosine) , 5);
}

vec3 scatter_dielectric(dielectric di, inout ray r, intersection_record rec, inout uint seed) {
  float ri = 0;
  if (rec.front_face) {
    ri = 1.0 / di.refraction_idx;
  } else {
    ri = di.refraction_idx;
  }

  vec3 unit_dir = normalize(r.direction);
  
  float cos_theta = min(dot(-unit_dir, rec.normal), 1.0);
  float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

  bool cannot_refract = ri * sin_theta > 1.0;
  vec3 direction;

  if (cannot_refract ||  reflectance(cos_theta, ri) > random_float(seed)) {
    direction = reflect(unit_dir, rec.normal);
  } else {
    direction = refract(unit_dir, rec.normal, ri);
  }

  r = ray(get_ray_point(rec.t, r), direction);
  return vec3(1.0) * di.albedo;
}

void scatter_ray(material m, inout vec3 color, inout ray r, intersection_record rec, inout uint seed) {
  if (m.material_type == MATERIAL_LAMBERTIAN) {
    color *= scatter_lambertian(material_as_lambertian(m), r, rec, seed);
  }
  else if (m.material_type == MATERIAL_METAL) {
    color *= scatter_metal(material_as_metal(m), r, rec, seed);
  }
  else if (m.material_type == MATERIAL_DIELECTRIC) {
    color *= scatter_dielectric(material_as_dielectric(m), r, rec, seed);
  }
}