enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCsBindings SHARED)
set_target_properties(OtherCsBindings PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)

target_sources(OtherCsBindings PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/type-extensions.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/set.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/native-string.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/assembly-loader.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop-interface.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/native-function-manager.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/logger.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/other-host.cs
)