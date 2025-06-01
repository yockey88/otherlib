add_dynamic_driver(pbrt_sandbox
  pbrt/pbrt_sandbox.cpp 
  pbrt/shape.cpp
  pbrt/integrator.cpp
  pbrt/scene_object.cpp
)

add_dynamic_driver(memory_sandbox
  memory/memory_sandbox.cpp
)

add_dynamic_driver(serialization_sandbox
  serialization/serialization_sandbox.cpp
)