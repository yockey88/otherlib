layout (std140) uniform camera_buffer {
  vec4 camera_position;
  vec4 camera_forward;
  /// near & far clip, padding x2
  vec4 camera_features;

  mat4 view_matrix;
  mat4 projection_matrix;
};

vec2 screen_to_ndc(ivec2 screen_coords, int screen_width, int screen_height) {
  float ndc_x = 2.0f * (screen_coords.x / float(screen_width)) - 1.0f;
  float ndc_y = 1.0f - 2.0f * (screen_coords.y / float(screen_height));
  return vec2(ndc_x, ndc_y);
}

vec4 ndc_to_view(vec2 ndc_coords) {
  vec4 homo = vec4(ndc_coords.x, ndc_coords.y, 0.f, 1.f);
  mat4 inv_projection = inverse(projection_matrix);
  vec4 view_coords = inv_projection * homo;

  if (view_coords.w != 0) {
    view_coords = view_coords / view_coords.w;
  }
  return view_coords;
}

vec3 view_to_world(vec4 view_coords) {
  vec4 homo = vec4(view_coords.xyz, 1);
  mat4 inv_view = inverse(view_matrix);
  vec4 world_coords = inv_view * homo;
  return world_coords.xyz;
}
