add_library(other_network STATIC)
target_sources(other_network
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/network_thread.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/session.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/session_protocol_handler.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/network/protocols/check_in_protocol.cpp"
)

target_include_directories(other_network 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/network>
)

target_link_directories(other_network PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_network PRIVATE other_core)

other_install_module(other_network "network")