include(CSharpUtilities)
add_library(OtherEditorCs SHARED)
set_target_properties(OtherEditorCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)

target_sources(OtherEditorCs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_window.cs
)
target_link_libraries(OtherEditorCs PUBLIC OtherCs OtherCsBindings)