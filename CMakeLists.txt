cmake_minimum_required(VERSION 3.27.0..4.0.2)
project(other VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Debug-AS")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_definitions("OTHER_ENVIRONMENT_WINDOWS" "HAS_SEH_EXCEPTIONS" "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
elseif(UNIX)
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_definitions("OTHER_ENVIRONMENT_LINUX")
  else()
    message(FATAL_ERROR "Unsupported UNIX platform")
  endif()
else()
  message(FATAL_ERROR "Unsupported platform")
endif()

add_subdirectory(extern)

if (MSVC)
  add_compile_definitions("/Zc:preprocessor")
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug-AS)
  message(STATUS "ASAN enabled")
  if(MSVC)
    add_compile_options("/fsanitize=address" "/EHsc")
  else()
    add_compile_options("-fsanitize=address")
  endif()
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug build enabled")
  if(MSVC)
    add_compile_options("/Zi" "/Od" "/D_DEBUG" "/EHsc")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  else()
    add_compile_options("-ggdb3" "-O0")
  endif()
else()
  message(STATUS "Release build enabled")
  if(MSVC)
    # set(CMAKE_MSVC_RUNTIME_CHECKS "")
    # add_compile_options("/O2" "/Gy" "/GL" "/DNDEBUG")
  else()
    add_compile_options("-O2")
  endif()
endif()

if (MSVC)
  set(VS_STARTUP_PROJECT "other_driver")
endif()

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
  add_subdirectory(development-drivers)
else()
  # Code to execute if the file does not exist
  message(STATUS "not including development drivers")
endif()

add_subdirectory(driver)