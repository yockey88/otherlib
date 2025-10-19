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

set(CMAKE_INSTALL_PREFIX "C:/OtherEnvironment" CACHE PATH "Installation directory" FORCE)
include(GNUInstallDirs)
include(cmake/other_install.cmake)
set(OTHER_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/other")
set(OTHER_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(OTHER_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
set(OTHER_INSTALL_CMAKEDIR "${CMAKE_INSTALL_PREFIX}/cmake")
set(OTHER_INSTALL_ASSEMBLIES "${CMAKE_INSTALL_PREFIX}/dotnet-assemblies")

option(BUILD_OTHER_TERMINAL "Build the other terminal application" OFF)
option(BUILD_OTHER_TESTS "Build the other tests" OFF)
option(BUILD_DEVELOPMENT_DRIVERS "Build development drivers" ON)
option(BUILD_RENDERING_DEVELOPMENT_DRIVER "Build the rendering development driver" OFF)
option(BUILD_EXAMPLE_APPLICATION "Build the example application" OFF)
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10+
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  
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
message(STATUS "IMGUI_INCLUDE_DIRS: ${IMGUI_INCLUDE_DIRS}")

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

set(OTHER_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(OTHER_EXTERN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)
set(OTHER_EXTERN_INCLUDE_DIRS 
  ${OTHER_EXTERN_DIR}/magic_enum
  ${OTHER_EXTERN_DIR}/args
  ${OTHER_EXTERN_DIR}/tomlplusplus
  ${OTHER_EXTERN_DIR}/spdlog
  ${OTHER_EXTERN_DIR}/glm
  ${OTHER_EXTERN_DIR}/refl
  ${OTHER_EXTERN_DIR}/flatbuffers
  ${OTHER_EXTERN_DIR}/asio
  ${OTHER_EXTERN_DIR}/spdlog
  ${OTHER_EXTERN_DIR}/sdl
  ${OTHER_EXTERN_DIR}/imgui
  ${OTHER_EXTERN_DIR}/glad/include
  ${OTHER_EXTERN_DIR}/assimp
  ${OTHER_EXTERN_DIR}/stb
  ${OTHER_EXTERN_DIR}/mio
  ${OTHER_EXTERN_DIR}/dotnet
  ${OTHER_EXTERN_DIR}/entt
  ${OTHER_EXTERN_DIR}/pybind11/include
  ${OTHER_EXTERN_DIR}/nativefiledialog/src
  ${OTHER_EXTERN_DIR}/json
  ${IMGUI_INCLUDE_DIRS}
)

set(DOTNET_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/dotnet" 
                             "${CMAKE_CURRENT_SOURCE_DIR}/extern/pybind11/include"
                             "${CMAKE_CURRENT_SOURCE_DIR}/extern/json"
                             "${CMAKE_CURRENT_SOURCE_DIR}/extern/mio"
                             "${CMAKE_CURRENT_SOURCE_DIR}/extern/nativefiledialog/src"
)
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

include(CMakePackageConfigHelpers)

if(EXISTS "development-drivers" AND IS_DIRECTORY "development-drivers")
  add_subdirectory(development-drivers)
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

other_install_external_headers("magic_enum" "${OTHER_EXTERN_DIR}/magic_enum")
other_install_external_headers("args" "${OTHER_EXTERN_DIR}/args")
other_install_external_headers("tomlplusplus" "${OTHER_EXTERN_DIR}/tomlplusplus")
other_install_external_headers("spdlog" "${OTHER_EXTERN_DIR}/spdlog")
other_install_external_headers("glm" "${OTHER_EXTERN_DIR}/glm")
other_install_external_headers("refl" "${OTHER_EXTERN_DIR}/refl")
other_install_external_headers("flatbuffers" "${OTHER_EXTERN_DIR}/flatbuffers")
other_install_external_headers("asio" "${OTHER_EXTERN_DIR}/asio")
other_install_external_headers("glad" "${OTHER_EXTERN_DIR}/glad/include")
other_install_external_headers("assimp" "${OTHER_EXTERN_DIR}/assimp")
other_install_external_headers("stb" "${OTHER_EXTERN_DIR}/stb")
other_install_external_headers("mio" "${OTHER_EXTERN_DIR}/mio")
other_install_external_headers("dotnet" "${OTHER_EXTERN_DIR}/dotnet")
other_install_external_headers("entt" "${OTHER_EXTERN_DIR}/entt")
other_install_external_headers("sdl" "${OTHER_EXTERN_DIR}/sdl")
other_install_external_headers("imgui" "${IMGUI_INCLUDE_DIRS}")
other_install_external_headers("pybind11" "${OTHER_EXTERN_DIR}/pybind11/include")
other_install_external_headers("nativefiledialog" "${OTHER_EXTERN_DIR}/nativefiledialog/src")
other_install_external_headers("json" "${OTHER_EXTERN_DIR}/json")
other_install_external_headers("python312" "${OTHER_EXTERN_DIR}/python312/include")

## Install external libraries
other_install_external_library("sdl3" "${OTHER_EXTERN_DIR}/sdl/lib")
other_install_external_library("assimp" "${OTHER_EXTERN_DIR}/assimp/lib")
other_install_external_library("glad" "${OTHER_EXTERN_DIR}/glad/lib")
other_install_external_library("nativefiledialog" "${OTHER_EXTERN_DIR}/nativefiledialog/lib")
other_install_external_library("flatbuffers" "${OTHER_EXTERN_DIR}/flatbuffers/lib")
other_install_external_library("python312" "${OTHER_EXTERN_DIR}/python312/libs")

install(
  FILES
    "${OTHER_EXTERN_DIR}/python312/python312.dll"
  DESTINATION "${OTHER_INSTALL_LIBDIR}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  set(DLL_CONFIG "debug")
else()
  set(DLL_CONFIG "release")
endif()

install(
  FILES
    "${OTHER_EXTERN_DIR}/sdl/lib/${DLL_CONFIG}/SDL3.dll"
  DESTINATION "${OTHER_INSTALL_BINDIR}"
  CONFIGURATIONS ${CMAKE_BUILD_TYPE}
  OPTIONAL
)

install(
  FILES
    "${OTHER_EXTERN_DIR}/assimp/lib/${DLL_CONFIG}/assimp-vc143-mt$<$<CONFIG:Debug,ProfileD>:d>.dll"
  DESTINATION "${OTHER_INSTALL_BINDIR}"
  CONFIGURATIONS ${CMAKE_BUILD_TYPE}
  OPTIONAL
)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/other-csharp-interop/resources/OtherCsBindings.runtimeconfig.json"
  DESTINATION "${OTHER_INSTALL_ASSEMBLIES}"
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  set(ASSEMBLY_CONFIG "Debug")
else()
  set(ASSEMBLY_CONFIG "Release")
endif()

install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/other-csharp/${ASSEMBLY_CONFIG}/"
  DESTINATION "${OTHER_INSTALL_ASSEMBLIES}"
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/otherlib-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/otherlib-config.cmake"
  INSTALL_DESTINATION "${OTHER_INSTALL_CMAKEDIR}"
  PATH_VARS OTHER_INSTALL_INCLUDEDIR OTHER_INSTALL_LIBDIR OTHER_INSTALL_BINDIR
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/otherlib-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/otherlib-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/otherlib-config-version.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_other_driver.cmake"
  DESTINATION "${OTHER_INSTALL_CMAKEDIR}"
)

export(
  EXPORT otherlib-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/otherlib-targets.cmake"
  NAMESPACE otherlib::
)

install(
  EXPORT otherlib-targets
  FILE otherlib-targets.cmake
  NAMESPACE otherlib::
  DESTINATION "${OTHER_INSTALL_CMAKEDIR}"
)