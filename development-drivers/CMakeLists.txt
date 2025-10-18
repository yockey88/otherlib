add_subdirectory(script-testing)


if (BUILD_DEVELOPMENT_DRIVERS)
  add_static_driver(rendering_dev
    rendering-dev/renderer.cpp
    rendering-dev/renderer_driver.cpp
  )

  add_static_driver(simulation_driver
    simulation/simulation.cpp
    simulation/simulation_driver.cpp
    simulation/main.cpp
  )

  add_static_driver(runtime_dev
    runtime-dev/runtime.cpp
    runtime-dev/main.cpp
  )
  add_static_driver(server_dev
    server-dev/server.cpp

    server-dev/server-ui/project-window.cpp
    server-dev/server-ui/project-creator.cpp
    server-dev/server-ui/server-ui.cpp

    server-dev/main.cpp
  )
endif()