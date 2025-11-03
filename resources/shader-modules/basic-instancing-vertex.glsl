layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitangent;
layout (location = 4) in vec2 OE_tex_coords;
layout (location = 5) in ivec4 OE_bone_ids;
layout (location = 6) in vec4 OE_bone_weights;

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

// Explicit bindings to match pipeline: model_buffer -> binding 1, camera_buffer -> binding 2, bone_buffer -> binding 3
layout (std140, binding = 1) uniform model_buffer {
  mat4 models[MAX_OBJECTS];
};

layout (std140, binding = 3) uniform bone_buffer {
  mat4 bones[MAX_OBJECTS];
  int use_bones;
};

bool has_bones() {
  return use_bones == 1;
}

out int OE_material_index;

void set_instance_id() {
  OE_material_index = gl_InstanceID;
}

mat4 get_camera_matrix() {
  return projection_matrix * view_matrix;
}

mat4 get_instance_model_matrix() {
  return models[gl_InstanceID];
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


vec3 get_normal_bone_contribations() {
  vec3 bone_normal = vec3(0.0);

  for (int i = 0; i < MAX_VERTEX_BONE_INFLUENCE; ++i) {
    if (OE_bone_ids[i] == -1) {
      continue;
    }
    if (OE_bone_ids[i] >= MAX_OBJECTS) {
      bone_normal = OE_normal;
      break;
    }

    mat3 bone3 = mat3(bones[OE_bone_ids[i]]);
    bone_normal += (bone3 * OE_normal) * OE_bone_weights[i];
  }

  return bone_normal;
}

vec4 get_transform_bone_contributions() {
  vec4 bone_position = vec4(0.f);

  for (int i = 0; i < MAX_VERTEX_BONE_INFLUENCE; ++i) {
    if (OE_bone_ids[i] == -1) {
      continue;
    }
    if (OE_bone_ids[i] >= MAX_OBJECTS) {
      bone_position = vec4(OE_position, 1.0);
      break;
    }

    bone_position += bones[OE_bone_ids[i]] * vec4(OE_position, 1.0) * OE_bone_weights[i];
  }

  return bone_position;
}