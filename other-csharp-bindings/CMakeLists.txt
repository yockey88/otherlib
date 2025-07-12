enable_language(CSharp)
include(CSharpUtilities)

add_library(Other SHARED)
set_target_properties(Other PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)

target_sources(Other PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/native-string.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/assembly-loader.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/other-host.cs
)