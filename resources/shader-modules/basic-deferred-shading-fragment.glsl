struct point_light {
  vec4 position;
  vec4 color;
};

struct direction_light {
  vec4 direction;
  vec4 color;
};

layout (std140) uniform camera_buffer {
  vec4 camera_position;
  vec4 camera_forward;

  /// near & far clip, defocus_angle padding x2
  vec4 camera_features;

  vec4 defocus_disk_u;
  vec4 defocus_disk_v;

  mat4 view_matrix;
  mat4 projection_matrix;
};

layout (std430) readonly buffer direction_light_buffer {
  direction_light direction_lights[];
};

layout (std430) readonly buffer point_light_buffer {
  point_light point_lights[];
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

uniform mat4 OE_light_space_matrix;
uniform vec3 OE_light_position;
uniform sampler2D OE_shadow_map;

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

vec4 calculate_lighting(vec3 diffuse, vec3 world_position, vec3 world_normal, float specular_reflect) {
  vec3 view_dir = normalize(camera_position.xyz - world_position);

  vec3 diffuse_specular = vec3(0);
  for (int i = 0; i < OE_num_point_lights; ++i) {
    vec3 light_dir = normalize(point_lights[i].position.xyz - world_position);
    vec3 diffuse = max(dot(world_normal, light_dir), 0.0) * diffuse * point_lights[i].color.rgb;
    
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(world_normal, halfway_dir), 0.0), 16.0);
    vec3 specular = point_lights[i].color.rgb * spec * specular_reflect;
    
    float attentuation = attenuate(length(point_lights[i].position.xyz - world_position));
    diffuse_specular += (diffuse * attentuation) + (specular * attentuation);
  }

  vec3 ambient = vec3(1.0);
  if (OE_num_direction_lights > 0) {
    vec3 light_dir = normalize(direction_lights[0].direction.xyz);
    ambient = direction_lights[0].color.rgb * max(dot(world_normal, light_dir), 0.0) * diffuse;
  }

  float shadow_calc = calculate_direction_light_shadow(world_position, world_normal);
  vec3 lighting = (ambient + (1.0 - shadow_calc) * diffuse_specular) * diffuse;
  return vec4(lighting, 1.0);
}

uniform sampler2D OE_gbuff_albedo;
uniform sampler2D OE_gbuff_normal;
uniform sampler2D OE_gbuff_position;