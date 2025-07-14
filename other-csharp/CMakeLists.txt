enable_language(CSharp)
include(CSharpUtilities)

add_library(OtherCs SHARED)
set_target_properties(OtherCs PROPERTIES
  DOTNET_SDK "Microsoft.NET.Sdk"
  DOTNET_TARGET_FRAMEWORK "net9.0"
  VS_GLOBAL_AllowUnsafeBlocks "true"
)

target_sources(OtherCs PRIVATE
)