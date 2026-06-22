// ============================================================================
//  smaa.glsl  ->  resources/shader-modules/smaa.glsl
//
//  Engine adapter for SMAA (Subpixel Morphological Anti-Aliasing) —
//  https://github.com/iryoku/smaa (MIT License)
//
//  This adapter is included by the per-pass SMAA wrapper shaders, which define
//  the following BEFORE the include:
//      #define SMAA_RT_METRICS  vec4(1/w, 1/h, w, h)   // via textureSize(...)
//      #define SMAA_PRESET_HIGH                        // or LOW / MEDIUM / ULTRA
//      #define SMAA_GLSL_4
//      #define SMAA_INCLUDE_PS 0   // in *.vert  (compile only the VS half)
//      #define SMAA_INCLUDE_VS 0   // in *.frag  (compile only the PS half)
// ============================================================================

#ifndef SMAA_RT_METRICS
#error "smaa.glsl: SMAA_RT_METRICS must be defined before #include (see the wrapper shaders)."
#endif

// Force the GLSL-4 backend of the upstream core (gather4, texture arrays, etc.).
#ifndef SMAA_GLSL_4
#define SMAA_GLSL_4
#endif

// A preset (or the individual SMAA_THRESHOLD / SMAA_MAX_SEARCH_STEPS / ...) must
// be set by the includer. Default to HIGH if none was provided.
#if !defined(SMAA_PRESET_LOW) && !defined(SMAA_PRESET_MEDIUM) && \
    !defined(SMAA_PRESET_HIGH) && !defined(SMAA_PRESET_ULTRA) && \
    !defined(SMAA_THRESHOLD)
#define SMAA_PRESET_HIGH
#endif

// ----------------------------------------------------------------------------
//  http://www.iryoku.com/smaa/
//
//  With SMAA_GLSL_4 defined above, SMAA.hlsl compiles as GLSL unchanged
//  and exposes:
//      SMAAEdgeDetectionVS
//      SMAALumaEdgeDetectionPS
//      SMAAColorEdgeDetectionPS
//      SMAABlendingWeightCalculationVS
//      SMAABlendingWeightCalculationPS
//      SMAANeighborhoodBlendingVS
//      SMAANeighborhoodBlendingPS
// ----------------------------------------------------------------------------
#include "shader-modules/smaa-core.glsl"
