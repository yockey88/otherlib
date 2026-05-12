add_executable(oecli)
target_include_directories(oecli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})  
target_sources(oecli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/other-environment-cli/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/other-environment-cli/commands.cpp)
target_link_libraries(oecli PRIVATE otherlib)