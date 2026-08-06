add_library(other_network STATIC)
target_sources(other_network
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/transport_provider.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/acknowledgement_list.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/network_thread.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer-mesh/packet_sink.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer-mesh/peer_state_machine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer-mesh/peer_actor.cpp"
)

target_include_directories(other_network 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/network>
)

target_link_directories(other_network PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_network PRIVATE other_core)

other_install_module(other_network "network")