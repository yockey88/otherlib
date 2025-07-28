layout (location = 0) in vec3 OE_position;
layout (location = 1) in vec3 OE_normal;
layout (location = 2) in vec3 OE_tangent;
layout (location = 3) in vec3 OE_bitanget;
layout (location = 4) in vec2 OE_tex_coords;

uniform mat4 OE_light_space_matrix;

layout (std140, binding = 1) uniform model_buffer {
  mat4 models[MAX_OBJECTS];
};

void main() {
  gl_Position = OE_light_space_matrix * models[gl_InstanceID] * vec4(OE_position, 1.0);
}