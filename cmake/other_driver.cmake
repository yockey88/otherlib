macro(add_driver_target type driver_name)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /utf-8")
  endif()

  set(COMPILE_DEFS "")
  if (MSVC)
    set(COMPILE_DEFS 
      "OTHER_ENVIRONMENT_WINDOWS" 
      ${BUILD_CONFIG_MACRO} 
      "NOMINMAX" 
      "WIN32_LEAN_AND_MEAN" 
      "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" 
      "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS"
    )
  elseif(UNIX)
    set(COMPILE_DEFS 
      "OTHER_ENVIRONMENT_UNIX" 
      ${BUILD_CONFIG_MACRO}
    )
  endif()

  target_compile_definitions(
    ${driver_name} 
      PRIVATE 
        "$<$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>:OTHER_ENVIRONMENT_DEBUG>"
        "$<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>>:OTHER_ENVIRONMENT_RELEASE>")
  set(driver_src_list "")
  foreach(src_file ${ARGN})
    set(src_file_full_path "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    list(APPEND driver_src_list "${src_file_full_path}")
  endforeach()

  if (${type} STREQUAL "static")
    add_executable(${driver_name})
    target_compile_definitions(${driver_name} PRIVATE "OTHER_APPLICATION" ${COMPILE_DEFS})
  elseif (${type} STREQUAL "dynamic")
    add_library(${driver_name} SHARED)
    target_compile_definitions(${driver_name} PRIVATE "OTHER_CLIENT" ${COMPILE_DEFS})
  else()
    message(FATAL_ERROR "Unknown driver type: ${type}. Use 'static' or 'dynamic'.")
  endif()

  message(STATUS "Adding [${type}] driver: ${driver_name} @ ${CMAKE_CURRENT_SOURCE_DIR}")
  target_sources(${driver_name} PUBLIC ${driver_src_list})
  target_link_libraries(${driver_name} PUBLIC otherlib::otherlib)
  if (${type} STREQUAL "static")
    target_link_libraries(${driver_name} PUBLIC otherlib_main)
  endif()
endmacro()

macro(add_static_driver driver_name)
  add_driver_target("static" ${driver_name} ${ARGN})
endmacro()

macro(add_dynamic_driver driver_name)
  add_driver_target("dynamic" ${driver_name} ${ARGN})
endmacro()