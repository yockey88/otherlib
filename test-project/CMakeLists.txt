add_library(net_sim_plugin SHARED plugins/network_sim.cpp)
target_link_libraries(net_sim_plugin PRIVATE otherlib)
target_compile_definitions(net_sim_plugin PRIVATE OTHER_PLUGIN_LIBRARY)

## the shipped MANET sample (Mode 4 exemplar); scenario source shared with the
##  deterministic test in tests/network
add_library(manet_sample_plugin SHARED plugins/manet_sample.cpp)
target_include_directories(manet_sample_plugin PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(manet_sample_plugin PRIVATE otherlib)
target_compile_definitions(manet_sample_plugin PRIVATE OTHER_PLUGIN_LIBRARY)