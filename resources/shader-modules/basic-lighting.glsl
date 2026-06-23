struct light {
  vec4 vector;
  vec4 color;
  float type; // 0 - directional, 1 - point light
};

layout (std140) buffer light_buffer {
  light lights[MAX_LIGHTS];
};

uniform mat4 OE_light_space_matrix;
uniform vec3 OE_light_position;
uniform sampler2D OE_shadow_map;
uniform int OE_num_lights;

float attenuate(float dist){ 
  dist *= DIST_FACTOR; 
  return 1.0f / (CONSTANT + LINEAR * dist + QUADRATIC * dist * dist); 
}

vec3 calc_direction_light(light light, vec3 diffuse_color, vec3 world_normal) {
  vec3 light_dir = normalize(-light.vector.xyz);
  return light.color.rgb * max(dot(world_normal, light_dir), 0.0) * diffuse_color;
}

vec3 calc_point_light(const light light, const vec3 world_normal, const vec3 world_position) {
  const vec3 direction = normalize(light.vector.xyz - world_position);
	const float dist_to_light = distance(light.vector.xyz, world_position);
	return max(dot(normalize(world_normal), direction), 0.0f) * 5 * attenuate(dist_to_light) * light.color.rgb;
}

float calculate_direction_light_shadow(vec3 world_position, vec3 world_normal) {
  vec4 light_space_position = OE_light_space_matrix * vec4(world_position, 1.0);
  
  vec3 proj_coords = light_space_position.xyz / light_space_position.w;
  proj_coords = proj_coords * 0.5 + 0.5;

  float closest_depth = texture(OE_shadow_map, proj_coords.xy).r;
  float curr_depth = proj_coords.z; 

  vec3 normal = normalize(world_normal);
  vec3 light_dir = normalize(OE_light_position - world_position);
  float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);

  float shadow = 0.0;
  vec2 texel = 1.0 / textureSize(OE_shadow_map, 0);
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcf_depth = texture(OE_shadow_map, proj_coords.xy + vec2(x, y) * texel).r;
      shadow += curr_depth - bias > pcf_depth ? 1.0 : 0.0;
    }
  }
  shadow /= 9.0;

  if (proj_coords.z > 1.0) {
    shadow = 0.0;
  }
  
  return shadow;
}