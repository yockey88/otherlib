add_library(other_network STATIC)
target_sources(other_network
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/frame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/transport_provider.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/acknowledgement_list.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/network_thread.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/memory/memory_fabric.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/tcp/connection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/tcp/tcp_transport_provider.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/network/udp/udp_transport_provider.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/packet_sink.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/peer_state_machine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/mesh_router.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/peer_mesh.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/peer_mesh_actor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/peer_mesh/provider_link_transport.cpp"
)

target_include_directories(other_network 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/network>
)

target_link_directories(other_network PRIVATE ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_network PRIVATE other_core)

other_install_module(other_network "network")