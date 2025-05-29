layout (std140) uniform ray_buffer {
  vec4 pixel00_loc;
  vec4 pixel_delta_u;
  vec4 pixel_delta_v;
  vec2 square_sample;
};

ray get_ray(vec2 pixel, vec2 sample_vec) {
  vec3 pixel00 = pixel00_loc.xyz;
  vec3 pixeldu = pixel_delta_u.xyz;
  vec3 pixeldv = pixel_delta_v.xyz;

  vec3 pixel_sample = pixel00 + 
          ((float(pixel.x) + sample_vec.x) * pixeldu) + 
          ((float(pixel.y) + sample_vec.y) * pixeldv);

  ray r;
  r.origin = camera_position.xyz;
  r.direction = normalize(pixel_sample - r.origin);
  return r;
}

vec3 cast_ray(ray r, interval range, int depth) {
  vec3 color = vec3(1.0);
  vec3 final_color = vec3(0.0);
  ray current_ray = r;

  for (int i = 0; i < depth; ++i) {
    intersection_record rec;
    intersect_object(current_ray, range, rec);
    if (!rec.hit) {
      float a = 0.5f * (normalize(current_ray.direction).y + 1.f);
      vec3 sky_grad = get_gradient(a, vec3(1.f, 1.f, 1.f), vec3(0.5f, 0.7f, 1.f));
      final_color += color * sky_grad;
      return final_color;
    }

    material mat = get_object_material(rec.idx);
    color *= mat.albedo; // mat.absorption

    vec3 hit_point = get_ray_point(rec.t, current_ray);
    
    uint seed = uint(gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * 1920 + i * 73 + rec.idx * 37);
    vec3 scatter_direction = random_cosine_hemisphere(rec.normal, seed);
    if (length(scatter_direction) < epsilon) {
      scatter_direction = rec.normal;
    }

    const float shadow_offset = 0.001f;
    current_ray.origin = hit_point + rec.normal * shadow_offset;
    current_ray.direction = scatter_direction;

    range.min = shadow_offset;
  }

  return color;
}