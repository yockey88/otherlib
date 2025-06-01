add_executable(other_terminal)
target_compile_definitions(other_terminal PRIVATE "OTHER_APPLICATION")
target_sources(other_terminal
    PRIVATE
        src/terminal/terminal_thread.cpp
        src/terminal/terminal.cpp

        src/other-terminal.cpp
)
target_include_directories(other_terminal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(other_terminal PRIVATE otherlib)