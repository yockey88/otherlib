add_subdirectory(other-build-tool)

add_executable(project-writer)
target_include_directories(project-writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(project-writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/project-writer/main.cpp)
target_link_libraries(project-writer PUBLIC otherlib)

add_executable(mesh-baker)
target_include_directories(mesh-baker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(mesh-baker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/mesh-baker/main.cpp)
target_link_libraries(mesh-baker PUBLIC otherlib)