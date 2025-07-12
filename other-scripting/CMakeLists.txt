add_library(other_scripting STATIC)
target_sources(other_scripting
  PRIVATE
    src/dotnet/host.cpp
)
target_include_directories(other_scripting 
  PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${DOTNET_INCLUDE_DIRECTORY}
)
target_link_directories(other_scripting PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_scripting PUBLIC other_core)