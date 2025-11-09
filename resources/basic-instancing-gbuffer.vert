#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 world_position;
out vec3 world_normal;

void main() {
  set_instance_id();

  mat4 model_matrix = get_instance_model_matrix();
  mat3 normal_mat = transpose(inverse(mat3(model_matrix)));

  mat4 bone_transform = get_bone_transform();
  mat3 bone_normal = transpose(inverse(mat3(bone_transform)));

  vec4 skinned_world_pos = model_matrix * bone_transform * get_local_position();
  world_position = skinned_world_pos.xyz;
  world_normal = normalize(normal_mat * bone_normal * get_normal().xyz);
  gl_Position = get_camera_matrix() * skinned_world_pos;
}