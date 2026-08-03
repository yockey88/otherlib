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
    src/cli/tools/build_environment.cpp
    src/cli/tools/create_project.cpp
    src/cli/tools/dev_common.cpp
    src/cli/tools/install_environment.cpp
    src/cli/tools/open_project.cpp
    src/cli/tools/run_driver.cpp
    src/cli/tools/test_runner.cpp
)
target_include_directories(other_cli
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/cli>
)
target_link_libraries(other_cli PUBLIC other_core)

## two front ends build from the same main.cpp, differentiated by OTHER_CLI_DEV_TOOLS:
##  oecli - the developer cli. Registers the source-tree workflow tools (run, build,
##    test, install, package) next to the project workflow; only meaningful inside a
##    build tree and never installed (unless OTHER_INSTALL_DEV_CLI opts in).
##  oecli_user - the user cli that ships with the SDK, still named oecli.exe but built
##    into other-cli/user/<config>/ so the two never collide. Project and engine tools
##    only; the developer tools stay unregistered.
## Both link otherlib for the engine-level tools (scene compile/decompile/info need the
##  scene component codecs); neither boots subsystems, so tools stay instant.
add_executable(oecli src/main.cpp)
target_compile_definitions(oecli PRIVATE OTHER_CLI_DEV_TOOLS)

add_executable(oecli_user src/main.cpp)
set_target_properties(oecli_user PROPERTIES
  OUTPUT_NAME "oecli"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/user"
)

foreach(cli_frontend oecli oecli_user)
  target_link_libraries(${cli_frontend} PRIVATE other_cli otherlib)
  ## other_core's third-party link deps (glad, xxhash) resolve by bare name
  target_link_directories(${cli_frontend} PRIVATE ${OTHER_LIBRARIES_PATH})
endforeach()

other_install_module(other_cli "cli")
if (OTHER_INSTALL_DEV_CLI)
  install(TARGETS oecli RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}")
else()
  install(TARGETS oecli_user RUNTIME DESTINATION "${OTHER_INSTALL_BINDIR}")
endif()
