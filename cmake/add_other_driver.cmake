## Driver-target helpers, usable both from the source tree and from an installed
## otherlib SDK (find_package(otherlib)). This is the single implementation;
## cmake/other_driver.cmake is a compatibility shim that includes this file.
##
## In-tree consumers link the build targets (otherlib / otherlib_main); installed
## consumers link the exported otherlib:: targets and get the SDK lib directories
## added so prebuilt third-party libs referenced by bare name (lua-5.4.4, SDL3,
## glad, ...) resolve at link time.
macro(add_driver_target type driver_name)
  if (${type} STREQUAL "static")
    add_executable(${driver_name})
    target_compile_definitions(${driver_name} PRIVATE "OTHER_APPLICATION")
  elseif (${type} STREQUAL "dynamic")
    add_library(${driver_name} SHARED)
    target_compile_definitions(${driver_name} PRIVATE "OTHER_CLIENT")
  else()
    message(FATAL_ERROR "Unknown driver type: ${type}. Use 'static' or 'dynamic'.")
  endif()

  set_target_properties(${driver_name} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

  if (MSVC)
    target_compile_options(${driver_name} PRIVATE /utf-8 /bigobj /MP /Zc:preprocessor)
    target_compile_definitions(${driver_name}
      PRIVATE
        "OTHER_ENVIRONMENT_WINDOWS"
        "HAS_SEH_EXCEPTIONS"
        "NOMINMAX"
        "WIN32_LEAN_AND_MEAN"
        "_WIN32_WINNT=0x0A00"
        "_CRT_SECURE_NO_WARNINGS"
        "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING"
        "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS"
    )
  endif()

  ## Profile maps to the release macros and ProfileD to the debug macros, matching
  ##  the in-tree configuration split
  target_compile_definitions(${driver_name}
    PRIVATE
      "$<$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>:OTHER_ENVIRONMENT_DEBUG>"
      "$<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>>:OTHER_ENVIRONMENT_RELEASE>"
      "$<$<OR:$<CONFIG:Profile>,$<CONFIG:ProfileD>>:TRACY_ENABLE>"
      "OTHER_PROJECT_FILE_TOML_FORMAT"
  )

  set(driver_src_list "")
  foreach(src_file ${ARGN})
    list(APPEND driver_src_list "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
  endforeach()

  message(STATUS "Adding [${type}] driver: ${driver_name} @ ${CMAKE_CURRENT_SOURCE_DIR}")

  target_include_directories(${driver_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_sources(${driver_name} PRIVATE ${driver_src_list})

  if (TARGET otherlib)
    ## building inside the otherlib source tree
    target_link_libraries(${driver_name} PUBLIC otherlib)
    if (${type} STREQUAL "static")
      target_link_libraries(${driver_name} PUBLIC otherlib_main)
    endif()
  else()
    ## consuming an installed SDK via find_package(otherlib)
    target_link_libraries(${driver_name} PUBLIC otherlib::otherlib)
    if (${type} STREQUAL "static")
      target_link_libraries(${driver_name} PUBLIC otherlib::otherlib_main)
    endif()
    if (DEFINED OTHERLIB_LIB_DIR)
      target_link_directories(${driver_name}
        PRIVATE
          "${OTHERLIB_LIB_DIR}"
          "${OTHERLIB_LIB_DIR}/$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>,debug,release>"
      )
    endif()
  endif()
endmacro()

macro(add_static_driver driver_name)
  add_driver_target("static" ${driver_name} ${ARGN})
endmacro()

macro(add_dynamic_driver driver_name)
  add_driver_target("dynamic" ${driver_name} ${ARGN})
endmacro()
