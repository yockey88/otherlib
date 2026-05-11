add_library(other_physics STATIC)

target_sources(other_physics
  PRIVATE
    src/physics_world/physics_world.cpp
    src/physics_world/physics_body.cpp

    # src/physics/backends/physx_api.cpp
    src/physics/backends/jolt_api.cpp

    src/physics/physics_api.cpp
    src/physics/physics_environment.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
  set(PHYSX_BUILD_TYPE "checked" CACHE STRING "PhysX build type")
else()
  set(PHYSX_BUILD_TYPE "release" CACHE STRING "PhysX build type")
endif()


target_include_directories(other_physics 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/physics>
)
target_precompile_headers(other_physics REUSE_FROM other_core)
target_link_directories(other_physics 
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}> 
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

set(PHYSX_LIBS
  PhysX_64
  PhysXCommon_64
  PhysXCooking_64
  PhysXExtensions_static_64
  PhysXFoundation_64
  PhysXGpu_64
  PhysXPvdSdk_static_64
)
# if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "ProfileD")
#   set(PHYSX_LIBS ${PHYSX_LIBS} PVDRuntime_64)
# endif()

target_link_libraries(other_physics 
  PRIVATE 
    ${PYTHON_LIB} 
    other_core 
    Jolt
)

other_install_module(other_physics "physics")