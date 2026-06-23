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

layout (std430) readonly buffer material_buffer {
  material materials[];
};

flat in int OE_material_index;

material get_instance_material() {
  return materials[OE_material_index];
}