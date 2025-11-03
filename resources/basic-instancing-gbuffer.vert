#include "shader-modules/basic-instancing-vertex.glsl"

out vec3 world_position;
out vec3 world_normal;

void main() {
  set_instance_id();

  mat4 model_matrix = get_instance_model_matrix();
  mat3 normal_mat = transpose(inverse(mat3(model_matrix)));

  if (has_bones()) {
    vec4 skinned_local = get_transform_bone_contributions();
    vec4 skinned_world = model_matrix * skinned_local;

    world_position = skinned_world.xyz;
    world_normal = normalize(normal_mat * get_normal_bone_contribations());
    gl_Position = get_camera_matrix() * skinned_world;
  } else {
    world_position = get_world_position().xyz;
    world_normal = normalize(normal_mat * OE_normal);
    gl_Position = get_camera_matrix() * vec4(world_position, 1.0);
  }
}