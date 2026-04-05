enable_language(CSharp)
include(CSharpUtilities)

add_library(DotnetTesting SHARED)
set_target_properties(DotnetTesting PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "false"
)

target_sources(DotnetTesting PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test-object.cs
)
target_link_libraries(DotnetTesting PRIVATE OtherCs)