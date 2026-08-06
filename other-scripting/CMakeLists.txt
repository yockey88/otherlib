add_library(other_scripting STATIC)
target_sources(other_scripting
PRIVATE
  src/bindings/native_logger.cpp

  src/dotnet/csproj_helpers.cpp
  src/dotnet/dotnet_memory.cpp
  src/dotnet/native_string.cpp
  src/dotnet/type_cache.cpp
  src/dotnet/dotnet_method.cpp
  src/dotnet/dotnet_field.cpp
  src/dotnet/dotnet_attribute.cpp
  src/dotnet/dotnet_type.cpp
  src/dotnet/dotnet_assembly.cpp
  src/dotnet/dotnet_object.cpp
  src/dotnet/dotnet_host.cpp

  src/lua/lua_sandbox.cpp
  src/lua/lua_host.cpp
  src/lua/lua_script.cpp

  src/script/script_object.cpp
  src/script/scripting_environment.cpp
)

set(LUA_LIB_DIRECTORY
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
  $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

target_include_directories(other_scripting 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/scripting>
)
target_link_directories(other_scripting
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
    $<BUILD_INTERFACE:${LUA_LIB_DIRECTORY}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)
target_link_libraries(other_scripting PRIVATE lua-5.4.4 other_core)

target_precompile_headers(other_scripting PRIVATE <sol/sol.hpp>)

other_install_module(other_scripting "scripting")