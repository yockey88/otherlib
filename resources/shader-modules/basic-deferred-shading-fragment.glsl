struct point_light {
  vec3 position;
  vec3 color;
};

struct material {
  vec3 diffuse_color;
  float diffuse_reflect;

  vec3 specular_color;
  float specular_reflect;

  float emissivity;
  float transparency;
  float shininess;
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

layout (std140) uniform point_light_buffer {
  point_light point_lights[MAX_POINT_LIGHTS];
};

struct direction_light {
  vec3 direction;
  vec3 color;
};

layout (std140) uniform direction_light_buffer {
  direction_light direction_lights[MAX_DIRECTION_LIGHTS];
};

layout (std140) uniform material_buffer {
  material materials[MAX_OBJECTS];
};

uniform int OE_num_point_lights;
uniform int OE_num_direction_lights;

in flat int OE_mat_idx;

material get_material() {
  return materials[OE_mat_idx];
}

float attenuate(float dist){ 
  dist *= DIST_FACTOR; 
  return 1.0f / (CONSTANT + LINEAR * dist + QUADRATIC * dist * dist); 
}


vec3 calc_direction_light(direction_light light, vec3 diffuse_color, vec3 world_normal) {
  vec3 light_dir = normalize(-light.direction);
  return light.color * max(dot(world_normal, light_dir), 0.0) * diffuse_color;
}

vec3 calc_point_light(const point_light light, const vec3 world_normal, const vec3 world_position) {
  const vec3 direction = normalize(light.position - world_position);
	const float dist_to_light = distance(light.position, world_position);
	const float attenuation = attenuate(dist_to_light);
	return max(dot(normalize(world_normal), direction), 0.0f) * 5 * attenuation * light.color;
}


vec4 calculate_lighting(vec3 diffuse, vec3 world_position, vec3 world_normal, float specular_reflect) {
  vec3 lighting = vec3(0);
  material mat = get_material();

  lighting = diffuse * 0.1; 
  vec3 view_dir = normalize(camera_position.xyz - world_position);

  for (int i = 0; i < OE_num_point_lights; ++i) {
    vec3 light_dir = normalize(point_lights[i].position - world_position);
    vec3 diffuse = max(dot(world_normal, light_dir), 0.0) * diffuse * point_lights[i].color;
    
    vec3 halfway_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(world_normal, halfway_dir), 0.0), 16.0);
    vec3 specular = point_lights[i].color * spec * specular_reflect;
    
    float attentuation = attenuate(length(point_lights[i].position - world_position));
    
    diffuse *= attentuation;
    specular *= attentuation;
    lighting += diffuse + specular;
  }

  for (int i = 0; i < OE_num_direction_lights; ++i) {
    vec3 light_dir = normalize(direction_lights[i].direction);
    lighting += direction_lights[i].color * max(dot(world_normal, light_dir), 0.0) * diffuse;
  }

  return vec4(lighting, 1.0);
}

uniform sampler2D OE_gbuff_albedo;
uniform sampler2D OE_gbuff_normal;
uniform sampler2D OE_gbuff_position;