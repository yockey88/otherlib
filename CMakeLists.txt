cmake_minimum_required(VERSION 4.0.2)

include(cmake/add_other_driver.cmake)
include(cmake/configure_version_header.cmake)

project(other VERSION 1.0.0 LANGUAGES CXX CSharp)
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

if (NOT CMAKE_CXX_FLAGS_PROFILE)
  set(CMAKE_CXX_FLAGS_PROFILE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "C++ flags for Profile builds." FORCE)
endif()
if (NOT CMAKE_CXX_FLAGS_PROFILED)
  set(CMAKE_CXX_FLAGS_PROFILED "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "C++ flags for ProfileD builds." FORCE)
endif()
if (NOT CMAKE_CSharp_FLAGS_PROFILE)
  set(CMAKE_CSharp_FLAGS_PROFILE "${CMAKE_CSharp_FLAGS_RELEASE}" CACHE STRING "C# flags for Profile builds." FORCE)
endif()
if (NOT CMAKE_CSharp_FLAGS_PROFILED)
  set(CMAKE_CSharp_FLAGS_PROFILED "${CMAKE_CSharp_FLAGS_DEBUG}" CACHE STRING "C# flags for ProfileD builds." FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

## default SDK location; overridable with -DCMAKE_INSTALL_PREFIX=<path>
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "C:/OtherEnvironment" CACHE PATH "Installation directory" FORCE)
endif()
include(GNUInstallDirs)
include(cmake/other_install.cmake)
set(OTHER_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/other")
set(OTHER_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(OTHER_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
set(OTHER_INSTALL_CMAKEDIR "${CMAKE_INSTALL_PREFIX}/cmake")
set(OTHER_INSTALL_ASSEMBLIES "${CMAKE_INSTALL_PREFIX}/dotnet-assemblies")
set(OTHER_INSTALL_RESOURCES "${CMAKE_INSTALL_PREFIX}/resources")
set(OTHER_INSTALL_LUA "${CMAKE_INSTALL_PREFIX}/lua")
set(OTHER_INSTALL_PROJECT "${CMAKE_INSTALL_PREFIX}/project")
set(OTHER_INSTALL_PROJECT_TEMPLATES "${CMAKE_INSTALL_PREFIX}/project/templates")

message(STATUS "[INSTALL PATHS]\n  Includedir: ${OTHER_INSTALL_INCLUDEDIR}\n  Libdir: ${OTHER_INSTALL_LIBDIR}\n  Bindir: ${OTHER_INSTALL_BINDIR}\n  CMakeDir: ${OTHER_INSTALL_CMAKEDIR}\n  Assemblies: ${OTHER_INSTALL_ASSEMBLIES}\n  Project: ${OTHER_INSTALL_PROJECT}")

### projects to build
option(BUILD_OTHER_TESTS "Build the other tests" OFF)
option(BUILD_SCRATCH_PROGRAMS "Build scratch-work/development brainstorming programs" OFF)
## every build tree produces both cli front ends (oecli with the source-tree developer
##  tools, oecli_user with the project workflow only); this only selects which of the
##  two `install`/`package` ship as the SDK's oecli.exe
option(OTHER_INSTALL_DEV_CLI "Ship the developer oecli (run/build/test/install/package) with the SDK instead of the user cli" OFF)
if (OTHER_INSTALL_DEV_CLI)
  message(STATUS "SDK cli flavor: developer (source-tree workflow tools included)")
else()
  message(STATUS "SDK cli flavor: user (project workflow only)")
endif()
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

set(BUILD_CONFIG_MACRO "$<$<CONFIG:Debug>:OTHER_ENVIRONMENT_DEBUG>$<$<CONFIG:Release>:OTHER_ENVIRONMENT_RELEASE>$<$<CONFIG:Profile>:OTHER_ENVIRONMENT_PROFILE>$<$<CONFIG:ProfileD>:OTHER_ENVIRONMENT_PROFILED>")
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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /bigobj")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0A00") # Windows 10+
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_USE_MATH_DEFINES")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")
  add_compile_definitions("_CRT_SECURE_NO_WARNINGS")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>:Debug>DLL")
else()
  message(FATAL_ERROR "Unix builds are not supported yet. Please use Windows development.")
endif()

add_subdirectory(extern)


set(OTHER_LIB_FAMILY "$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>,debug,release>")
set(OTHER_LIBRARIES_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/sdl/lib/${OTHER_LIB_FAMILY}"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/glad/lib/${OTHER_LIB_FAMILY}"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/jolt/lib/${OTHER_LIB_FAMILY}"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/box3d/lib/${OTHER_LIB_FAMILY}"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/tinyxml2/lib/${OTHER_LIB_FAMILY}"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/xxHash/lib/${OTHER_LIB_FAMILY}"
)

set(ASSIMP_NAME "assimp-vc143-mt")
set(OTHER_LIBRARIES_PATH ${OTHER_LIBRARIES_PATH} 
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/assimp/lib"
)
  
add_compile_definitions("$<$<OR:$<CONFIG:Profile>,$<CONFIG:ProfileD>>:TRACY_ENABLE>" "$<$<OR:$<CONFIG:Profile>,$<CONFIG:ProfileD>>:TRACY_CALLSTACK=10>")

set(OTHER_ROOT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(OTHER_EXTERN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/extern)
set(OTHER_EXTERN_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/other/extern")
set(OTHER_EXTERN_INCLUDE_DIRS
  ${OTHER_EXTERN_DIR}/magic_enum
  ${OTHER_EXTERN_DIR}/args
  ${OTHER_EXTERN_DIR}/tomlplusplus
  ${OTHER_EXTERN_DIR}/spdlog
  ${OTHER_EXTERN_DIR}/glm
  ${OTHER_EXTERN_DIR}/refl
  ${OTHER_EXTERN_DIR}/asio
  ${OTHER_EXTERN_DIR}/spdlog
  ${OTHER_EXTERN_DIR}/sdl
  ${OTHER_EXTERN_DIR}/imgui
  ${OTHER_EXTERN_DIR}/glad/include
  ${OTHER_EXTERN_DIR}/assimp
  ${OTHER_EXTERN_DIR}/stb
  ${OTHER_EXTERN_DIR}/miniaudio
  ${OTHER_EXTERN_DIR}/dotnet
  ${OTHER_EXTERN_DIR}/entt
  ${OTHER_EXTERN_DIR}/pybind11/include
  ${OTHER_EXTERN_DIR}/json
  ${OTHER_EXTERN_DIR}/sol2
  ${OTHER_EXTERN_DIR}/sol2/lua
  ${OTHER_EXTERN_DIR}/sol2/sol
  ${OTHER_EXTERN_DIR}/jolt
  ${OTHER_EXTERN_DIR}/box3d/include
  ${OTHER_EXTERN_DIR}/tinyxml2
  ${OTHER_EXTERN_DIR}/xxHash/include
  ${IMGUI_INCLUDE_DIRS}
)
set(OTHER_EXTERN_INCLUDE_DIRS ${OTHER_EXTERN_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/extern/tracy")
set(OTHER_EXTERN_INSTALL_INCLUDE_DIRS
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/magic_enum
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/args
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/tomlplusplus
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/spdlog
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/glm
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/refl
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/asio
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/spdlog
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/sdl
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/imgui
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/glad/include
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/assimp
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/stb
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/miniaudio
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/dotnet
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/entt
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/pybind11/include
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/json
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/sol2
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/sol2/lua
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/sol2/sol
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/tinyxml2
  ${OTHER_EXTERN_INSTALL_INCLUDEDIR}/xxHash/include
  ${OTHER_EXTERN_INCLUDE_DIRS}
)

set(DOTNET_INCLUDE_DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/dotnet"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/pybind11/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/json"
)
set(DOTNET_LIBRARIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/dotnet/lib")
set(DOTNET_LIBRARIES nethost)

set(PYTHON_EXECUTABLE "C:\\Python313\\python")
set(PYBIND11_FINDPYTHON ON) 
set(PYBIND11_PYTHON_VERSION "3.13")

find_package(Threads REQUIRED)

add_compile_definitions(OTHER_PROJECT_FILE_TOML_FORMAT)

add_subdirectory(other-core)
add_subdirectory(other-network)
add_subdirectory(other-renderer)
add_subdirectory(other-ui)
add_subdirectory(other-scripting)
add_subdirectory(other-csharp-interop)
add_subdirectory(other-physics)
add_subdirectory(other-audio)
add_subdirectory(other-scene)
add_subdirectory(other-csharp)
add_subdirectory(other-cli)
add_subdirectory(otherlib)

add_subdirectory(other-editor)
add_subdirectory(other-server)

add_subdirectory(script-testing)
if (BUILD_OTHER_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/scratch)
  add_subdirectory(scratch)
endif()
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test-project)
  add_subdirectory(test-project)
endif()
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/spacesim)
  add_subdirectory(spacesim)
endif()

include(CMakePackageConfigHelpers)
other_install_external_headers("magic_enum" "${OTHER_EXTERN_DIR}/magic_enum")
other_install_external_headers("args" "${OTHER_EXTERN_DIR}/args")
other_install_external_headers("tomlplusplus" "${OTHER_EXTERN_DIR}/tomlplusplus")
other_install_external_headers("spdlog" "${OTHER_EXTERN_DIR}/spdlog")
other_install_external_headers("glm" "${OTHER_EXTERN_DIR}/glm")
other_install_external_headers("refl" "${OTHER_EXTERN_DIR}/refl")
other_install_external_headers("asio" "${OTHER_EXTERN_DIR}/asio")
other_install_external_headers("glad" "${OTHER_EXTERN_DIR}/glad/include")
other_install_external_headers("assimp" "${OTHER_EXTERN_DIR}/assimp")
other_install_external_headers("stb" "${OTHER_EXTERN_DIR}/stb")
other_install_external_headers("miniaudio" "${OTHER_EXTERN_DIR}/miniaudio")
other_install_external_headers("dotnet" "${OTHER_EXTERN_DIR}/dotnet")
other_install_external_headers("entt" "${OTHER_EXTERN_DIR}/entt")
other_install_external_headers("sdl" "${OTHER_EXTERN_DIR}/sdl")
other_install_external_headers("imgui" "${IMGUI_INCLUDE_DIRS}")
other_install_external_headers("pybind11" "${OTHER_EXTERN_DIR}/pybind11/include")
other_install_external_headers("json" "${OTHER_EXTERN_DIR}/json")
other_install_external_headers("python312" "${OTHER_EXTERN_DIR}/python312/include")
other_install_external_headers("sol2" "${OTHER_EXTERN_DIR}/sol2")
other_install_external_headers("jolt" "${OTHER_EXTERN_DIR}/jolt/Jolt")
other_install_external_headers("tinyxml2" "${OTHER_EXTERN_DIR}/tinyxml2")
other_install_external_headers("xxHash" "${OTHER_EXTERN_DIR}/xxHash/include")

# ## Install external libraries
other_install_external_library("sdl3" "${OTHER_EXTERN_DIR}/sdl/lib")
other_install_external_library("assimp" "${OTHER_EXTERN_DIR}/assimp/lib")
other_install_external_library("glad" "${OTHER_EXTERN_DIR}/glad/lib")
other_install_external_library("python312" "${OTHER_EXTERN_DIR}/python312/libs")
other_install_external_library("jolt" "${OTHER_EXTERN_DIR}/jolt/lib")
other_install_external_library("sol2" "${OTHER_EXTERN_DIR}/sol2/lib")
other_install_external_library("tinyxml2" "${OTHER_EXTERN_DIR}/tinyxml2/lib")
other_install_external_library("xxHash" "${OTHER_EXTERN_DIR}/xxHash/lib")
  
set(DLL_CONFIG "$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>,debug,release>")
set(ASSEMBLY_CONFIG "$<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>,Debug,Release>")
install(
  FILES
    "${OTHER_EXTERN_DIR}/python312/python312.dll"
    "${OTHER_EXTERN_DIR}/sdl/lib/${DLL_CONFIG}/SDL3.dll"
    "${OTHER_EXTERN_DIR}/assimp/lib/assimp-vc143-mt.dll"
    "${OTHER_EXTERN_DIR}/sol2/lib/lua-5.4.4.dll"
    "${OTHER_EXTERN_DIR}/jolt/bin/${DLL_CONFIG}/Jolt.dll"
    "${OTHER_EXTERN_DIR}/xxHash/bin/${DLL_CONFIG}/xxhash.dll"
  DESTINATION "${OTHER_INSTALL_BINDIR}/${DLL_CONFIG}"
  OPTIONAL
)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/other-csharp-interop/resources/OtherCsBindings.runtimeconfig.json"
  DESTINATION "${OTHER_INSTALL_ASSEMBLIES}"
)

if (NOT EXISTS "${OTHER_INSTALL_PROJECT}")
  file(MAKE_DIRECTORY "${OTHER_INSTALL_PROJECT}")
endif()

install(
  DIRECTORY "${CMAKE_BINARY_DIR}/other-csharp/${ASSEMBLY_CONFIG}/"
  DESTINATION "${OTHER_INSTALL_ASSEMBLIES}"
)

install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/templates/"
  DESTINATION "${OTHER_INSTALL_PROJECT_TEMPLATES}"
)

### now install resources (shaders, pipelines, boot scripts, fonts, ui layouts, ...)
## everything the runtime references from resources/ ships with the SDK; models are
##  local development content and stay out of the install
set(OTHERLIB_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources")
install(
  DIRECTORY "${OTHERLIB_RESOURCES}/"
  DESTINATION "${OTHER_INSTALL_RESOURCES}"
  PATTERN "models" EXCLUDE
)

## the engine's lua runtime (bridge, console, interfaces); the dotnet/lua hosts default
##  scripting.other-lua-directory to <prefix>/lua
install(
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/other-lua-interop/"
  DESTINATION "lua"
  FILES_MATCHING
  PATTERN "*.lua"
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
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/other_driver.cmake"
  DESTINATION "${OTHER_INSTALL_CMAKEDIR}"
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

include(cmake/other_packaging.cmake)