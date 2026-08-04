#include "shader-modules/camera.glsl"

layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitangent;
layout (location = 4) in vec2 OE_tex_coords;
/// the vertex stream is all floats and every attribute binds as GL_FLOAT — an ivec4
///  here would read reinterpreted float bits. declare what arrives; cast per use
layout (location = 5) in vec4 OE_bone_ids;
layout (location = 6) in vec4 OE_bone_weights;

layout (std140) uniform model_buffer {
  mat4 models[MAX_OBJECTS];
};

layout (std140) uniform bone_buffer {
  mat4 bones[MAX_BONES];
  int use_bones;
};

bool has_bones() {
  return use_bones == 1;
}

flat out int OE_material_index;
out vec2 OE_frag_uv;

/// per-instance id for the material block plus the uv forward (vertex location 4 finally
/// has a consumer — the material texture slots)
void set_instance_id() {
  OE_material_index = gl_InstanceID;
  OE_frag_uv = OE_tex_coords;
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

mat4 get_bone_transform() {
  if (!has_bones()) {
    return mat4(1.f);
  }

  mat4 bone_transform = mat4(0.f);
  bool influenced = false;

  for (int i = 0; i < MAX_VERTEX_BONE_INFLUENCE; ++i) {
    int id = int(OE_bone_ids[i]);
    if (id < 0 || id >= MAX_BONES) {
      continue;
    }
    bone_transform += bones[id] * OE_bone_weights[i];
    influenced = true;
  }

  /// uninfluenced vertices keep bind pose instead of collapsing through mat4(0)
  return influenced ? bone_transform : mat4(1.f);
}