struct material {
  vec3 albedo;
  float shininess;
  float absorption;
};

layout (std140) uniform material_buffer {
  material materials[MAX_MATERIALS];
};