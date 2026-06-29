#include "shader-modules/camera.glsl"

layout(location = 0) in vec3 OE_position;
layout(location = 1) in vec3 OE_normal;

uniform mat4  OE_model;
uniform float OE_outline_width;

void main() {
  mat4 mvp = get_camera_matrix() * OE_model;
  vec4 clip   = mvp * vec4(OE_position, 1.0);
  if (OE_outline_width <= 0.0) { 
    gl_Position = clip; 
    return; 
  }

  vec3 world_n = normalize(mat3(OE_model) * OE_normal);
  vec4 clip_n  = get_camera_matrix() * vec4(world_n, 0.0);
  
  vec2 ndc_dir = normalize(clip_n.xy);
  vec2 px_to_ndc = vec2(2.0) / vec2(OE_viewport_size);    
  
  clip.xy += ndc_dir * px_to_ndc * OE_outline_width * clip.w;
  gl_Position = clip;
}