#ifdef WITH_GEOMETRY
#else
in vec3 frag_position;
in vec3 frag_normal;
#endif

float attenuate(float dist){ 
  dist *= DIST_FACTOR; 
  return 1.0f / (CONSTANT + LINEAR * dist + QUADRATIC * dist * dist); 
}

struct point_light {
  vec3 position;
  vec3 color;
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

struct material {
  vec3 diffuse_color;
  float diffuse_reflect;

  vec3 specular_color;
  float specular_reflect;

  float emissivity;
  float transparency;
  float shininess;
};

layout (std140) uniform material_buffer {
  material materials[MAX_MATERIALS];
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

uniform int num_point_lights;
uniform int num_direction_lights;
uniform int material_index;

out vec4 frag_color;

vec3 calc_point_light(const point_light light) {
  const vec3 direction = normalize(light.position - frag_position);
	const float dist_to_light = distance(light.position, frag_position);
	const float attenuation = attenuate(dist_to_light);
	return max(dot(normalize(frag_normal), direction), 0.0f) * 5 * attenuation * light.color;
}

vec3 calc_direction_light(const direction_light light, const material mat) {
  vec3 light_dir = normalize(-light.direction);
  return light.color * max(dot(frag_normal, light_dir), 0.0) * mat.diffuse_color;
}

void main() {
  vec3 color = vec3(0);
  for (int i = 0; i < num_point_lights; ++i) {
    color += calc_point_light(point_lights[i]);
  }

  material mat = materials[material_index];
  for (int i = 0; i < num_direction_lights; ++i) {
    color += calc_direction_light(direction_lights[i], mat);
  }

  vec3 spec = mat.specular_reflect * mat.specular_color;
  vec3 diff = mat.diffuse_reflect * mat.diffuse_color;
  color = (diff + spec) * color + clamp(mat.emissivity, 0, 1) * mat.diffuse_color;

  frag_color = vec4(color, 1.0); 
}