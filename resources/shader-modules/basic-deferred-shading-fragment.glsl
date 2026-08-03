#include "shader-modules/camera.glsl"
#include "shader-modules/simulation-environment.glsl"
#include "shader-modules/basic-lighting.glsl"
#include "shader-modules/scene-lighting.glsl"

/// deferred resolve inputs: the lighting itself lives in scene-lighting.glsl, shared with the
/// forward transparent pass so both paths shade identically
uniform sampler2D OE_gbuff_albedo;
uniform sampler2D OE_gbuff_normal;
uniform sampler2D OE_gbuff_position;
uniform sampler2D OE_gbuff_depth;
