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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
  add_compile_definitions("OTHER_ENVIRONMENT_WINDOWS" ${BUILD_CONFIG_MACRO} "HAS_SEH_EXCEPTIONS" "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
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
  set(OTHER_LIBRARIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/debug" "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/debug")
else()
  set(OTHER_LIBRARIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/release" "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/release")
endif()

set(OTHER_EXTERN_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/magic_enum
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/args
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/tomlplusplus
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/spdlog
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/pybind11/include
)

add_compile_definitions(SPDLOG_USE_STD_FORMAT=1)
add_subdirectory(otherlib)

if(EXISTS "development-drivers" AND IS_DIRECTORY "development-drivers")
  # Code to execute if the file exists
  message(STATUS "including development drivers")
  add_subdirectory(development-drivers)
else()
  # Code to execute if the file does not exist
  message(STATUS "not including development drivers")
endif()

add_subdirectory(driver)

add_subdirectory(tests)