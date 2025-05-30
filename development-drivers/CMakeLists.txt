macro(add_driver folder_name driver_name driver_src_list)
  if(NOT TARGET ${driver_name} AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${folder_name}")
    add_library(${driver_name} SHARED)

    target_compile_definitions(${driver_name} PRIVATE OTHER_CLIENT)
    target_include_directories(${driver_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${folder_name})

    target_sources(${driver_name} PRIVATE ${driver_src_list})
    target_link_libraries(${driver_name} PRIVATE otherlib)
  else()
    message(STATUS "Driver ${driver_name} already exists, skipping creation.")
  endif()
endmacro()

add_driver(pbrt_sandbox pbrt
  pbrt/pbrt_sandbox.cpp 
  pbrt/shape.cpp
  pbrt/integrator.cpp
  pbrt/scene_object.cpp
)

add_driver(memory_sandbox memory
  memory/memory_sandbox.cpp
)
