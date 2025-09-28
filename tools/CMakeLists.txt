add_executable(project-writer)
target_include_directories(project-writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(project-writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/project-writer/main.cpp)
target_link_libraries(project-writer PUBLIC otherlib)