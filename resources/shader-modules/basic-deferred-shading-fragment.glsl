struct point_light {
  vec4 position;
  vec4 color;
};

struct direction_light {
  vec4 direction;
  vec4 color;
};

layout (std140, binding = 2) uniform camera_buffer {
  vec4 camera_position;
  vec4 camera_forward;

  /// near & far clip, defocus_angle padding x2
  vec4 camera_features;

  vec4 defocus_disk_u;
  vec4 defocus_disk_v;

  mat4 view_matrix;
  mat4 projection_matrix;
};

layout (std430, binding = 1) readonly buffer point_light_buffer {
  point_light point_lights[];
};

layout (std430, binding = 0) readonly buffer direction_light_buffer {
  direction_light direction_lights[];
};

uniform int OE_num_point_lights;
uniform int OE_num_direction_lights;

float attenuate(float dist){ 
  dist *= DIST_FACTOR; 
  return 1.0f / (CONSTANT + LINEAR * dist + QUADRATIC * dist * dist); 
}

vec3 calc_direction_light(direction_light light, vec3 diffuse_color, vec3 world_normal) {
  vec3 light_dir = normalize(-light.direction.xyz);
  return light.color.rgb * max(dot(world_normal, light_dir), 0.0) * diffuse_color;
}

vec3 calc_point_light(const point_light light, const vec3 world_normal, const vec3 world_position) {
  const vec3 direction = normalize(light.position.xyz - world_position);
	const float dist_to_light = distance(light.position.xyz, world_position);
	return max(dot(normalize(world_normal), direction), 0.0f) * 5 * attenuate(dist_to_light) * light.color.rgb;
}

vec4 calculate_lighting(vec3 diffuse, vec3 world_position, vec3 world_normal, float specular_reflect) {
  vec3 lighting = vec3(0);

  lighting = diffuse * 0.1; 
  vec3 view_dir = normalize(camera_position.xyz - world_position);

  for (int i = 0; i < OE_num_point_lights; ++i) {
    vec3 light_dir = normalize(point_lights[i].position.xyz - world_position);
    vec3 diffuse = max(dot(world_normal, light_dir), 0.0) * diffuse * point_lights[i].color.rgb;
    
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(world_normal, halfway_dir), 0.0), 16.0);
    vec3 specular = point_lights[i].color.rgb * spec * specular_reflect;
    
    float attentuation = attenuate(length(point_lights[i].position.xyz - world_position));
    
    diffuse *= attentuation;
    specular *= attentuation;
    lighting += diffuse + specular;
  }

  for (int i = 0; i < OE_num_direction_lights; ++i) {
    vec3 light_dir = normalize(direction_lights[i].direction.xyz);
    lighting += direction_lights[i].color.rgb * max(dot(world_normal, light_dir), 0.0) * diffuse;
  }

  return vec4(lighting, 1.0);
}

uniform sampler2D OE_gbuff_albedo;
uniform sampler2D OE_gbuff_normal;
uniform sampler2D OE_gbuff_position;