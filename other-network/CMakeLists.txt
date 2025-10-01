add_library(other_network STATIC)
target_sources(other_network
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/network_thread.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/session.cpp"
)

target_include_directories(other_network 
  PUBLIC 
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_directories(other_network PUBLIC ${OTHER_LIBRARIES_PATH})

target_link_libraries(other_network PRIVATE other_core)