layout(std140) uniform simulation_environment_buffer {
  /// unit vector TO sun, w = angular radius
  vec4 sun_direction;
  // rgb = radiance, w = intensity scale
  vec4 sun_color;
  vec4 ambient_color;
  // rgb, w = turbidity
  vec4 zenith_color;
  vec4 horizon_color;
  vec4 ground_color;
  vec4 world_min;  //< xyz = AABB min
  vec4 world_max;  //< xyz = AABB max, w = exposure
};

vec3 world_extent() {
  return world_max.xyz - world_min.xyz;
}

vec3 oe_sky_radiance(vec3 dir) {
  float up = clamp(dir.y, 0.0, 1.0);
  
  // ground below the horizon, graded sky above
  vec3 sky = mix(horizon_color.rgb, zenith_color.rgb, pow(up, 0.5));
  sky = mix(ground_color.rgb, sky, step(0.0, dir.y));
  
  // sun_direction.w = angular radius in radians
  float cd = dot(normalize(dir), normalize(sun_direction.xyz));
  float disk = smoothstep(cos(sun_direction.w * 2.0), cos(sun_direction.w), cd);

  return sky + (sun_color.rgb * sun_color.w * disk);
}

vec3 oe_sky_irradiance_up(vec3 world_pos) {
  float h = clamp((world_pos.y - world_min.y) / max(world_extent().y, 1e-3), 0.0, 1.0);
  
  vec3  dome = mix(horizon_color.rgb, zenith_color.rgb, 0.5 + 0.5 * h);
  vec3  sun  = sun_color.rgb * sun_color.w * max(sun_direction.y, 0.0) * (1.0 - cos(sun_direction.w));
  vec3  haze = horizon_color.rgb * (1.0 - h) * 0.25;
  return (dome + sun + haze);
}

vec3 oe_world_to_volume(vec3 p) {
  return clamp((p - world_min.xyz) / world_extent(), vec3(0.0), vec3(1.0));
}

ivec3 oe_world_to_voxel(vec3 world_pos, ivec3 dim) {
  return ivec3(oe_world_to_volume(world_pos) * vec3(dim));
}

vec3 oe_voxel_to_world(ivec3 voxel, ivec3 dim) {
  vec3 uvw = (vec3(voxel) + 0.5) / vec3(dim);
  return world_min.xyz + uvw * world_extent();
}

vec3 oe_voxel_size(ivec3 dim) {
  return world_extent() / vec3(dim);
}