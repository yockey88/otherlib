layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitanget;
layout (location = 4) in vec2 OE_tex_coords;
/// all-float vertex stream (see basic-geometry.glsl) — bone ids arrive as floats
layout (location = 5) in vec4 OE_bone_ids;
layout (location = 6) in vec4 OE_bone_weights;

out vec3 world_position;
out vec3 world_normal;
out int OE_material_index;

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

layout (std140) uniform bone_buffer {
  mat4 bones[MAX_BONES];
};

uniform mat4 u_model_matrix;
uniform int u_rigged;

mat4 get_camera_matrix() {
  return projection_matrix * view_matrix;
}

mat4 get_instance_model_matrix() {
  return u_model_matrix;
}

vec4 get_local_position() {
  return vec4(OE_position, 1.0);
}

vec4 get_world_position() {
  return get_instance_model_matrix() * get_local_position();
}

mat4 get_instance_mvp_matrix() {
  return get_camera_matrix() * get_instance_model_matrix();
}

vec4 get_normal() {
  return vec4(OE_normal, 1.0);
}

bool is_all_zeros_precision(mat4 m, float epsilon) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (abs(m[i][j]) > epsilon) {
        return false;
      }
    }
  }
  return true;
}

bool is_all_zeros_precision(mat3 m, float epsilon) {
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (abs(m[i][j]) > epsilon) {
        return false;
      }
    }
  }
  return true;
}

mat3 get_normal_bone_contribations() {
  if (u_rigged == 0) {
    return mat3(1.0);
  }

  mat3 bone_normal = mat3(0.0);
  for (int i = 0; i < MAX_VERTEX_BONE_INFLUENCE; ++i) {
    int id = int(OE_bone_ids[i]);
    if (id < 0 || id >= MAX_BONES) {
      break;
    }
    bone_normal += mat3(bones[id]) * OE_bone_weights[i];
  }

  return bone_normal;
}

mat4 get_bone_transform() {
  if (u_rigged == 0) {
    return mat4(1.0);
  }

  mat4 transform = mat4(0.0);
  for (int i = 0; i < MAX_VERTEX_BONE_INFLUENCE; ++i) {
    int id = int(OE_bone_ids[i]);
    if (id < 0 || id >= MAX_BONES) {
      break;
    }
    transform += bones[id] * OE_bone_weights[i];
  }

  return transform;
}
  
void main() {
  mat4 model_matrix = get_instance_model_matrix();
  mat3 normal_mat = transpose(inverse(mat3(model_matrix)));

  mat4 bone_transform = get_bone_transform();
  mat3 bone_normal = get_normal_bone_contribations();

  vec4 skinned_world_pos = model_matrix * bone_transform * get_local_position();
  vec3 skinned_normal = normal_mat * bone_normal * get_normal().xyz;

  world_position = skinned_world_pos.xyz;
  world_normal = skinned_normal;
  gl_Position = get_camera_matrix() * skinned_world_pos;
}