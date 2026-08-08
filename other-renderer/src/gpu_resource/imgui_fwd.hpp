/**
 * \file gpu_resource/imgui_fwd.hpp
 **/
#ifndef OTHER_RENDERER_GPU_RESOURCE_IMGUI_FWD_HPP
#define OTHER_RENDERER_GPU_RESOURCE_IMGUI_FWD_HPP

/// mirrors imgui.h's ImTextureID so this header avoids pulling in all of imgui.
/// must stay in sync if imconfig.h ever overrides ImTextureID.
#ifndef ImTextureID
typedef unsigned long long ImU64;
typedef ImU64 ImTextureID;
#endif

typedef unsigned int ImWchar32;
typedef unsigned short ImWchar16;
#ifdef IMGUI_USE_WCHAR32
typedef ImWchar32 ImWchar;
#else
typedef ImWchar16 ImWchar;
#endif

#endif  // OTHER_RENDERER_GPU_RESOURCE_IMGUI_FWD_HPP
