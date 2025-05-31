macro(add_driver driver_name folder_name)
  message(STATUS "Adding driver: ${driver_name} with folder: ${folder_name}")
  
  set(driver_src_list "")
  foreach(src_file ${ARGN})
    if(NOT src_file MATCHES "^${folder_name}/")
      message(FATAL_ERROR "Source file '${src_file}' does not start with folder name '${folder_name}'")
    endif()

    # get_filename_component(src_file_path "${src_file}" PATH)
    # get_filename_component(src_file_name "${src_file}" NAME)

    set(src_file_full_path "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    list(APPEND driver_src_list "${src_file_full_path}")
  endforeach()

  add_library(${driver_name} SHARED)

  target_compile_definitions(${driver_name} PRIVATE OTHER_CLIENT)
  target_include_directories(${driver_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${folder_name})
  
  target_sources(${driver_name} PRIVATE ${driver_src_list})
  target_link_libraries(${driver_name} PRIVATE otherlib)
endmacro()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pbrt)
  add_driver(pbrt_sandbox pbrt
    pbrt/pbrt_sandbox.cpp 
    pbrt/shape.cpp
    pbrt/integrator.cpp
    pbrt/scene_object.cpp
  )
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/memory)
  add_driver(memory_sandbox memory
    memory/memory_sandbox.cpp
  )
endif()
