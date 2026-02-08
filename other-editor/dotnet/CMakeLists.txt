include(CSharpUtilities)
add_library(OtherEditorCs SHARED)
set_target_properties(OtherEditorCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set_target_properties(OtherEditorCs PROPERTIES
    VS_GLOBAL_DebugType "portable"
    VS_GLOBAL_OptimizeFor "Debug"
  )
else()
  set_target_properties(OtherEditorCs PROPERTIES
    VS_GLOBAL_DebugType "pdbonly"
    VS_GLOBAL_OptimizeFor "Release"
  )
endif()

target_sources(OtherEditorCs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_window.cs
)
target_link_libraries(OtherEditorCs PUBLIC OtherCs OtherCsBindings)