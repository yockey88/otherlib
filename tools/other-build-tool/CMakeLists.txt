enable_language(CSharp)
include(CSharpUtilities)

add_executable(OtherBuildTool)
set_target_properties(OtherBuildTool PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "false"
)

target_sources(OtherBuildTool PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/other-build-tool.cs
)
target_link_libraries(OtherBuildTool PRIVATE OtherCs)