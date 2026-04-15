enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCs SHARED)
set_target_properties(OtherCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/generators/native-function-generator.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/serializable-field.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/debug.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/behavior-descriptor.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/other-object.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/other-behavior.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/filesystem.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/matrix.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/vector.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/events.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/input.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/events.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/time.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/driver.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/network/network.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/ui.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/ui_window.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/UI/inspector.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/components.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/transform_access.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/transform.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-behavior.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-object-handle.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene-object.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/project-configuration.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/other-tool.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/build-tool/build-tool.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/util/log.cs
)
target_link_libraries(OtherCs PUBLIC OtherCsBindings)