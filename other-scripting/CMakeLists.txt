add_library(other_scripting STATIC)
target_sources(other_scripting
  PRIVATE
    src/bindings/native_logger.cpp

    src/dotnet/dotnet_memory.cpp
    src/dotnet/native_string.cpp
    src/dotnet/type_cache.cpp

    src/dotnet/dotnet_type.cpp
    src/dotnet/dotnet_assembly.cpp
    src/dotnet/host.cpp

    src/script/scripting_environment.cpp
)
target_include_directories(other_scripting 
  PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${DOTNET_INCLUDE_DIRECTORY}
)
target_link_directories(other_scripting PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_scripting PUBLIC other_core)