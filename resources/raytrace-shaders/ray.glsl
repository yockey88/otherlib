layout (std140) uniform ray_buffer {
  vec4 pixel00_loc;
  vec4 pixel_delta_u;
  vec4 pixel_delta_v;
};

ray get_ray(vec2 pixel, vec2 sample_vec, inout uint seed) {
  vec3 pixel00 = pixel00_loc.xyz;
  vec3 pixeldu = pixel_delta_u.xyz;
  vec3 pixeldv = pixel_delta_v.xyz;

  vec3 pixel_sample = pixel00 + 
          ((float(pixel.x) + sample_vec.x) * pixeldu) + 
          ((float(pixel.y) + sample_vec.y) * pixeldv);

  ray r;
  if (camera_features.z <= 0) {
    r.origin = camera_position.xyz;
  } else {
    r.origin = sample_defocus_disk(seed);
  }
  r.direction = normalize(pixel_sample - r.origin);
  return r;
}

vec3 get_gradient(float a, vec3 col1, vec3 col2) {
  return (1.0 - a) * col1 + a * col2;
}

vec3 cast_ray(ray r, interval range, int depth) {
  vec3 color = vec3(1.0);
  vec3 final_color = vec3(0.0);
  ray current_ray = r;

  uint base_seed = uint(gl_GlobalInvocationID.x * 1973 + 
                       gl_GlobalInvocationID.y * 2917 + 
                       gl_GlobalInvocationID.z * 4093);

  for (int i = 0; i < depth; ++i) {
    intersection_record rec;
    
    uint seed = base_seed + uint(i * 7919) + uint(depth * 5417);
    seed ^= frame_number * 3571;

    intersect_object(current_ray, range, rec);
    if (!rec.hit) {
      float a = normalize(current_ray.direction).y + 1.f;
      final_color += color * get_gradient(a, vec3(1.f, 1.f, 1.f), vec3(0.5f, 0.7f, 1.f));
      return final_color;
    }

    material mat = get_object_material(rec.idx);
    vec3 hit_point = get_ray_point(rec.t, current_ray);

    scatter_ray(mat, color, current_ray, rec, seed);

    float shadow_bias = 0.0001f;
    current_ray.origin = hit_point + rec.normal * shadow_bias;

    range.min = 0.001f;
  }

  return color * get_gradient(0.5, vec3(1.f, 1.f, 1.f), vec3(0.5f, 0.7f, 1.f));
}