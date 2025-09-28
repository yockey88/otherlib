add_subdirectory(script-testing)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pbrt)
  add_dynamic_driver(pbrt_sandbox
    pbrt/pbrt_sandbox.cpp 
    pbrt/shape.cpp
    pbrt/integrator.cpp
    pbrt/scene_object.cpp
  )
endif()

add_dynamic_driver(serialization_sandbox
  serialization/serialization_sandbox.cpp
)

add_dynamic_driver(math_physics_dev
  math-physics-dev/math-physics-driver.cpp
)

add_static_driver(memory_sandbox
  memory/memory.cpp
  memory/memory_sandbox.cpp
)

add_static_driver(rendering_dev
  rendering-dev/renderer.cpp
  rendering-dev/renderer_driver.cpp
)

add_static_driver(simulation_driver
  simulation/simulation.cpp
  simulation/simulation_driver.cpp
  simulation/main.cpp
)

add_dynamic_driver(scripting_dev
  scripting-dev/scripting_driver.cpp
)

add_static_driver(editor_dev
  editor-dev/editor.cpp
  editor-dev/main.cpp
)
add_static_driver(runtime_dev
  runtime-dev/runtime.cpp
  runtime-dev/main.cpp
)
add_static_driver(server_dev
  server-dev/network_thread.cpp
  server-dev/other_client.cpp
  
  server-dev/server.cpp
  server-dev/main.cpp
)