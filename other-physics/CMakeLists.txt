add_library(other_physics STATIC)
target_sources(other_physics
  PRIVATE
    src/physics_world/physics_world.cpp
    src/physics_world/physics_body.cpp
    src/physics_world/physics_shape.cpp

    src/physics/backends/jolt_api.cpp

    src/physics/physics_api.cpp
    src/physics/physics_environment.cpp
)

# ABI-sensitive defines the prebuilt extern/jolt binaries (5.5.1) were built with;
#  PUBLIC so every TU that includes a Jolt header sees the exact same layout
target_compile_definitions(other_physics
  PUBLIC
    JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
    JPH_PROFILE_ENABLED
    JPH_OBJECT_STREAM
    JPH_SHARED_LIBRARY
    JPH_DEBUG_RENDERER
)

target_include_directories(other_physics
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_INCLUDEDIR}/physics>
)
target_link_directories(other_physics
  PUBLIC
    $<BUILD_INTERFACE:${OTHER_LIBRARIES_PATH}>
    $<INSTALL_INTERFACE:${OTHER_INSTALL_LIBDIR}>
)

target_link_libraries(other_physics
  PRIVATE
    other_core
    Jolt
)

other_install_module(other_physics "physics")
