enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCsBindings SHARED)
set_target_properties(OtherCsBindings PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)

target_sources(OtherCsBindings PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/set.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/type-extensions.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/garbage-collector.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dotnet-object.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/attribute/other-attribute.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/attribute/interop-binding.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/attribute/native-class.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/native-function.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/logger-binding.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/native-string.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/native-array.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/assembly-loader.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/memory.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/type-interface.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/managed-object.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/native-function-manager.cs
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/other-host.cs
)