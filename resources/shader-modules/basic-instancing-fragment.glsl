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
flat in int OE_material_index;

material get_instance_material() {
  return materials[OE_material_index];
}

float attenuate(float dist){ 
  dist *= DIST_FACTOR; 
  return 1.0f / (CONSTANT + LINEAR * dist + QUADRATIC * dist * dist); 
}

vec3 calc_point_light(const point_light light, const vec3 world_normal, const vec3 world_position) {
  const vec3 direction = normalize(light.position - world_position);
	const float dist_to_light = distance(light.position, world_position);
	const float attenuation = attenuate(dist_to_light);
	return max(dot(normalize(world_normal), direction), 0.0f) * 5 * attenuation * light.color;
}

vec3 calculate_point_light_color(material mat, const vec3 world_normal, const vec3 world_position) {
  vec3 color;
  for (int i = 0; i < OE_num_point_lights; ++i) {
    color += calc_point_light(point_lights[i], world_normal, world_position);
  }
  return color;
}

vec3 calc_direction_light(const direction_light light, const material mat, const vec3 world_normal) {
  vec3 light_dir = normalize(-light.direction);
  return light.color * max(dot(world_normal, light_dir), 0.0) * mat.diffuse_color;
}

vec3 calculate_direction_light_color(material mat, const vec3 world_normal) {
  vec3 color;
  for (int i = 0; i < OE_num_direction_lights; ++i) {
    color += calc_direction_light(direction_lights[i], mat, world_normal);
  }
  return color;  
}