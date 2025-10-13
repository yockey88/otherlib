cmake_minimum_required(VERSION 3.27.0..4.0.2)

include(cmake/add_other_driver.cmake)

project(other VERSION 0.1.0 LANGUAGES CXX CSharp)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Profile;ProfileD" CACHE STRING "Build types" FORCE)

if (NOT CMAKE_SHARED_LINKER_FLAGS_PROFILE)
  set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Flags used by the linker during all build types." FORCE)
endif()
if (NOT CMAKE_SHARED_LINKER_FLAGS_PROFILED)
  set(CMAKE_SHARED_LINKER_FLAGS_PROFILED "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING "Flags used by the linker during all build types." FORCE)
endif()

if (NOT CMAKE_EXE_LINKER_FLAGS_PROFILE)
  set(CMAKE_EXE_LINKER_FLAGS_PROFILE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "Flags used by the linker during all build types." FORCE)
endif()
if (NOT CMAKE_EXE_LINKER_FLAGS_PROFILED)
  set(CMAKE_EXE_LINKER_FLAGS_PROFILED "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING "Flags used by the linker during all build types." FORCE)
endif()

## default to Release build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(BUILD_OTHER_TERMINAL "Build the other terminal application" OFF)
option(BUILD_OTHER_TESTS "Build the other tests" OFF)
option(BUILD_DEVELOPMENT_DRIVERS "Build development drivers" ON)
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

message(STATUS "using compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} @ [${CMAKE_CXX_COMPILER}]")

set(BUILD_CONFIG_MACRO "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD_CONFIG_MACRO "OTHER_ENVIRONMENT_DEBUG")
elseif (CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  set(BUILD_CONFIG_MACRO "OTHER_ENVIRONMENT_PROFILED")
elseif (CMAKE_BUILD_TYPE STREQUAL "Profile")
  set(BUILD_CONFIG_MACRO "OTHER_ENVIRONMENT_PROFILE")
else()
  set(BUILD_CONFIG_MACRO "OTHER_ENVIRONMENT_RELEASE")
endif()
message(STATUS "Build configuration: ${BUILD_CONFIG_MACRO}")

if (NOT CMAKE_BUILD_TYPE)
  message(FATAL_ERROR "CMAKE_BUILD_TYPE is not set. Please specify a build type (Debug, Release).")
endif()

set(BUILD_PLATFORM "")
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(BUILD_PLATFORM "WINDOWS")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(BUILD_PLATFORM "LINUX")
else()
  message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}. Please use Windows, Linux")
endif()

set(COMPILE_OPTIONS "")
if(BUILD_PLATFORM STREQUAL "WINDOWS")
  add_compile_definitions("OTHER_ENVIRONMENT_WINDOWS" ${BUILD_CONFIG_MACRO} "HAS_SEH_EXCEPTIONS" "NOMINMAX")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10+
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")

  if (DISALLOW_OTHER_PLUGINS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL Debug OR CMAKE_BUILD_TYPE STREQUAL ProfileD)
    message(STATUS "Debug build enabled")
    if (CMAKE_BUILD_TYPE STREQUAL Debug-AS)
      message(STATUS "Address Sanitizer enabled")
      add_compile_options("/fsanitize=address")
    endif()
    
    add_compile_options("/D_CRT_SECURE_NO_WARNINGS")
    add_compile_options("/Od" "/Zi" "/RTC1" "/D_DEBUG")
  elseif(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL Profile)
    message(STATUS "Release build enabled")
    add_compile_options("/DNDEBUG")
  endif()

else()
  # add_compile_definitions("OTHER_ENVIRONMENT_UNIX" ${BUILD_CONFIG_MACRO})

  # if(CMAKE_BUILD_TYPE MATCHES Debug)
  #   message(STATUS "Debug build enabled")
  #   message(STATUS "ASAN enabled")

  #   add_compile_options("-g3" "-O0" "-fsanitize=address")
  # elseif(CMAKE_BUILD_TYPE MATCHES Release)
  #   message(STATUS "Release build enabled")
    
  #   add_compile_options("-Ox")
  # endif()
  message(FATAL_ERROR "Unix builds are not supported yet. Please use Windows development.")
endif()

add_subdirectory(extern)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  set(OTHER_LIBRARIES_PATH 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/debug" 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/debug" 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/assimp/lib/debug"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/nativefiledialog/lib/debug"
  )
  set(ASSIMP_NAME "assimp-vc143-mtd")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "Profile")
  set(OTHER_LIBRARIES_PATH 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/release" 
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/release"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/assimp/lib/release"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/nativefiledialog/lib/release"
  )
  set(ASSIMP_NAME "assimp-vc143-mt")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Profile" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  add_compile_definitions("TRACY_ENABLE" "TRACY_CALLSTACK=10")
endif()
set(OTHER_LIBRARIES_PATH ${OTHER_LIBRARIES_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/extern/flatbuffers/lib")

set(OTHER_EXTERN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)

set(DOTNET_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/dotnet")
set(DOTNET_LIBRARIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/dotnet/lib")
set(DOTNET_LIBRARIES nethost)

set(PYTHON_EXECUTABLE "C:\\Python313\\python")
set(PYBIND11_FINDPYTHON ON) 
set(PYBIND11_PYTHON_VERSION "3.13")

find_package(Threads REQUIRED)

add_subdirectory(other-core)
add_subdirectory(other-network)

add_subdirectory(other-renderer)

add_subdirectory(other-scripting)
add_subdirectory(other-csharp-interop)

add_subdirectory(other-scene)
add_subdirectory(other-csharp)

add_subdirectory(otherlib)

if(EXISTS "development-drivers" AND IS_DIRECTORY "development-drivers")
  add_subdirectory(development-drivers)
endif()

if (BUILD_OTHER_TERMINAL)
  message(STATUS "Building other terminal application")
  add_subdirectory(other-terminal)
else()
  message(STATUS "Skipping other terminal application build")
endif()

add_subdirectory(driver)

if (BUILD_OTHER_TESTS)
  message(STATUS "Building other tests")
  enable_testing()
  add_subdirectory(tests)
else()
  message(STATUS "Skipping other tests build")
endif()

add_subdirectory(tools)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/scratch)
  add_subdirectory(scratch)
endif()
