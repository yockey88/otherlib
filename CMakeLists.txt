cmake_minimum_required(VERSION 3.27.0..4.0.2)
project(other VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;Releas" CACHE STRING "Build types" FORCE)

option(DISALLOW_OTHER_PLUGINS "Build the library statically and without the framework for loading plugins" OFF)
if (DISALLOW_OTHER_PLUGINS)
  message(STATUS "Disallowing other plugins")
  add_compile_definitions("OTHER_DISALLOW_OTHER_PLUGINS")
else()
  message(STATUS "Allowing other plugins")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

set(BUILD_CONFIG_MACRO "")
if (CMAKE_BUILD_TYPE MATCHES Debug)
  set(BUILD_CONFIG_MACRO "OTHER_DEBUG_BUILD")
else()
  set(BUILD_CONFIG_MACRO "OTHER_RELEASE_BUILD")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "No build type specified, defaulting to Release")
endif()

if(MSVC)
  add_compile_definitions("OTHER_ENVIRONMENT_WINDOWS" ${BUILD_CONFIG_MACRO} "HAS_SEH_EXCEPTIONS" "NOMINMAX")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10+
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
  add_compile_definitions("/Zc:preprocessor")

  if (DISALLOW_OTHER_PLUGINS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  endif()


  if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build enabled")
    message(STATUS "ASAN enabled")

    add_compile_options("/Zi" "/Od" "/D_DEBUG" "/EHsc" "/fsanitize=address")
    add_compile_definitions("_CRT_SECURE_NO_WARNINGS")
  elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Release build enabled")
    
    add_compile_options("/DNDEBUG")
  endif()

elseif(UNIX)
  # add_compile_definitions("OTHER_ENVIRONMENT_UNIX" ${BUILD_CONFIG_MACRO})

  # if(CMAKE_BUILD_TYPE MATCHES Debug)
  #   message(STATUS "Debug build enabled")
  #   message(STATUS "ASAN enabled")

  #   add_compile_options("-g3" "-O0" "-fsanitize=address")
  # elseif(CMAKE_BUILD_TYPE MATCHES Release)
  #   message(STATUS "Release build enabled")
    
  #   add_compile_options("-Ox")
  # endif()
  message(FATAL_ERROR "Unix builds are not supported yet. Please use Windows or MacOS for development.")
endif()

add_subdirectory(extern)

if (CMAKE_BUILD_TYPE MATCHES Debug-AS OR CMAKE_BUILD_TYPE MATCHES Debug)
  set(OTHER_LIBRARIES_PATH 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/debug" 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/debug"
  )
else()
  set(OTHER_LIBRARIES_PATH 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/release" 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/release"
  )
endif()
set(OTHER_LIBRARIES_PATH ${OTHER_LIBRARIES_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/extern/flatbuffers/lib")

set(OTHER_EXTERN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

find_package(Threads REQUIRED)

add_subdirectory(other-core)
add_subdirectory(other-renderer)
add_subdirectory(other-scene)
add_subdirectory(otherlib)

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

  set(driver_src_list "")
  foreach(src_file ${ARGN})
    set(src_file_full_path "${CMAKE_CURRENT_SOURCE_DIR}/${src_file}")
    list(APPEND driver_src_list "${src_file_full_path}")
  endforeach()

  message(STATUS "Adding [${type}] driver: ${driver_name} w/ include directory: ${CMAKE_CURRENT_SOURCE_DIR}")

  target_include_directories(${driver_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_sources(${driver_name} PUBLIC ${driver_src_list})
  target_link_libraries(${driver_name} PUBLIC otherlib)
endmacro()

macro(add_static_driver driver_name)
  add_driver_target("static" ${driver_name} ${ARGN})
endmacro()

macro(add_dynamic_driver driver_name)
  add_driver_target("dynamic" ${driver_name} ${ARGN})
endmacro()


if(EXISTS "development-drivers" AND IS_DIRECTORY "development-drivers")
  add_subdirectory(development-drivers)
endif()

add_subdirectory(other-terminal)
add_subdirectory(driver)

add_subdirectory(tests)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/scratch)
  add_subdirectory(scratch)
endif()