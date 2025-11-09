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

in vec3 world_normal;
in vec3 world_position;

uniform float light_ambient_intensity;
uniform float light_diffuse_intensity;
uniform vec3 light_position;
uniform vec3 light_ambient_color;
uniform vec3 light_diffuse_color;
uniform vec3 light_specular_color;

uniform float dir_light_ambient_intensity;
uniform float dir_light_diffuse_intensity;
uniform vec3 dir_light_direction;
uniform vec3 dir_light_ambient_color;
uniform vec3 dir_light_diffuse_color;
uniform vec3 dir_light_specular_color;

uniform vec3 obj_ambient_color;
uniform vec3 obj_diffuse_color;
uniform vec3 obj_specular_color;
uniform float shininess;

out vec4 frag_color;

void main() {
  vec3 norm = normalize(world_normal);

  vec3 light_direction = normalize(light_position - world_position);
  float diff = max(dot(norm, light_direction), 0.0);

  vec3 view_dir = normalize(light_position - camera_position.xyz);
  vec3 reflect_dir = reflect(-light_direction, norm);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);

  vec3 pl_ambient = light_ambient_intensity * light_ambient_color * obj_ambient_color;
  vec3 pl_diffuse = light_diffuse_intensity * light_diffuse_color * (diff * obj_diffuse_color);
  vec3 pl_specular = light_specular_color * (spec * obj_specular_color);

  vec3 dir_light_dir = normalize(-dir_light_direction);
  float dir_diff = max(dot(norm, dir_light_dir), 0.0);

  vec3 dir_reflect_dir = reflect(dir_light_direction, norm);
  float dir_spec = pow(max(dot(view_dir, dir_reflect_dir), 0.0), shininess);

  vec3 dir_ambient = dir_light_ambient_intensity * dir_light_ambient_color * obj_ambient_color;
  vec3 dir_diffuse = dir_light_diffuse_intensity * dir_light_diffuse_color * (dir_diff * obj_diffuse_color);
  vec3 dir_specular = dir_light_specular_color * (dir_spec * obj_specular_color);

  vec3 color = pl_ambient + pl_diffuse + pl_specular + dir_ambient + dir_diffuse + dir_specular;
  frag_color = vec4(color, 1.0);
}