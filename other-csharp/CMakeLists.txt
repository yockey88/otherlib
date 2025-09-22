enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCs SHARED)
set_target_properties(OtherCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
)

target_sources(OtherCs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/debug/logger.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/attribute/serializable-field.cs

  ${CMAKE_CURRENT_SOURCE_DIR}/src/object/other-object.cs
)
target_link_libraries(OtherCs PUBLIC OtherCsBindings)