enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCs SHARED)
set_target_properties(OtherCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
  VS_GLOBAL_Nullable "enable"
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set_target_properties(OtherCs PROPERTIES
    VS_GLOBAL_DebugType "portable"
    VS_GLOBAL_OptimizeFor "Debug"
  )
else()
  set_target_properties(OtherCs PROPERTIES
    VS_GLOBAL_DebugType "pdbonly"
    VS_GLOBAL_OptimizeFor "Release"
  )
endif()

target_sources(OtherCs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/fnv.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/exceptions.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/serializable-field.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/debug.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/behavior-descriptor.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/other-object.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/other-behavior.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/filesystem.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/int_vec2.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/int_vec3.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/int_vec4.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vec2.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vec3.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vec4.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/quaternion.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/matrix.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/events.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/input.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/events.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/time.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/driver.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/project.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/network/network.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/raw-vertex.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/vertex.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/raw-index.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/index.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/texture.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/ui.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/ui_window.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/inspector.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/other-abi.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/native-component.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/native-field.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/component.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/transform.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/render-component.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-object-handle.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-behavior.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-object.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/project-configuration.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/other-tool.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/build-tool/build-tool.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/project-tool.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/util/log.cs
)
target_link_libraries(OtherCs PUBLIC OtherCsBindings)

## add package reference for Microsoft.Build and Microsoft.Build.Locator
target_link_libraries(OtherCs PUBLIC Microsoft.Build Microsoft.Build.Locator Microsoft.Build.Utilities.Core)