struct material {
  vec3 diffuse_color;
  float diffuse_reflect;

  vec3 specular_color;
  float specular_reflect;

  vec3 emissive_color;
  float emissivity;
  
  float transparency;
  float shininess;
};

/// why does removing binding = 2 here break this?
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

layout (std430) readonly buffer material_buffer {
  material materials[];
};

uniform int OE_num_point_lights;
uniform int OE_num_direction_lights;
flat in int OE_material_index;

material get_instance_material() {
  return materials[OE_material_index];
}