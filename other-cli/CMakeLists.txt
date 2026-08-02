###
##  Other Environment CLI
##
##  other_cli - the tool library. Depends only on other_core so it can be linked both by
##    the standalone executable and into otherlib, which is what makes every tool
##    invocable from inside a running environment (editor console, drivers, plugins)
##    via other::cli::run(...).
##  oecli - the standalone command line front end.
##
add_library(other_cli STATIC)
target_sources(other_cli
  PRIVATE
    src/cli/environment.cpp
    src/cli/process.cpp
    src/cli/tool_registry.cpp
    src/cli/tools/create_project.cpp
    src/cli/tools/open_project.cpp
)
target_include_directories(other_cli
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/cli>
)
target_link_libraries(other_cli PUBLIC other_core)

add_executable(oecli src/main.cpp)
target_link_libraries(oecli PRIVATE other_cli)
## other_core's third-party link deps (glad, xxhash) resolve by bare name
target_link_directories(oecli PRIVATE ${OTHER_LIBRARIES_PATH})

other_install_module(other_cli "cli")
install(TARGETS oecli RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}")
