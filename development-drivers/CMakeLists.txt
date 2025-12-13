add_subdirectory(script-testing)


if (BUILD_DEVELOPMENT_DRIVER)
  add_static_driver(runtime_dev
    runtime-dev/behavior_tree.cpp
    runtime-dev/runtime.cpp
    runtime-dev/main.cpp
  )
endif()

if (BUILD_RENDERING_DEVELOPMENT_DRIVER)
  add_static_driver(rendering_dev
    rendering-dev/renderer.cpp
    rendering-dev/renderer_driver.cpp
  )
endif()

if (BUILD_EXAMPLE_APPLICATION)
  add_static_driver(simulation_driver
    simulation/simulation.cpp
    simulation/simulation_driver.cpp
    simulation/main.cpp
  )
endif()