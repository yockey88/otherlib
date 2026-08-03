/// mirrors the [materials.layout] block the pipeline declares in its TOML — the pipeline
/// owns this ABI, so editing the layout means editing this struct, in one place (std430:
/// vec4 @0, vec3 @16 with roughness packed into its pad @28, metalness @32, stride 48)
struct material {
  vec4 base_color;
  vec3 emissive_color;
  float roughness;
  float metalness;
};

layout (std430) readonly buffer material_buffer {
  material materials[];
};

uniform sampler2D OE_mat_base_color_map;
uniform sampler2D OE_mat_normal_map;

flat in int OE_material_index;
in vec2 OE_frag_uv;

material get_instance_material() {
  return materials[OE_material_index];
}

/// fallback slots are always bound (1x1 white when untextured), so the multiply is a no-op
/// for untextured materials — same math, zero shader variants
vec4 material_base_color() {
  return materials[OE_material_index].base_color * texture(OE_mat_base_color_map, OE_frag_uv);
}
