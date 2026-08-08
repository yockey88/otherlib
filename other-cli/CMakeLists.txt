###
## other_cli - tool library (only depends on other_core), linked into otherlib so every tool
##   is invocable from inside a running environment too; oecli - the standalone cli front end.
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

## two front ends from one main.cpp via OTHER_CLI_DEV_TOOLS: oecli adds the dev workflow
##   (build/test/install/package); oecli_user ships in the SDK, built to user/<config> to avoid collision.
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
