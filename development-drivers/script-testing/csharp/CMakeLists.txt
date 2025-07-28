enable_language(CSharp)
include(CSharpUtilities)

add_library(DotnetTesting SHARED)
set_target_properties(DotnetTesting PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "false"
)

target_sources(DotnetTesting PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ecma-frmt-testing.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/other-object-development.cs
)