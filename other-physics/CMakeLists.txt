add_library(other_physics STATIC)
target_sources(other_physics
  PRIVATE
    src/physics_world/physics_world.cpp
    src/physics_world/physics_body.cpp
    src/physics_world/physics_shape.cpp

    src/physics/backends/jolt_api.cpp
    src/physics/backends/box3d_api.cpp

    src/physics/physics_api.cpp
    src/physics/physics_environment.cpp
)

# ABI-sensitive defines matching the prebuilt extern/jolt 5.5.1 binaries; no Jolt header escapes
#  this module, so PRIVATE is safe — must become PUBLIC if a header ever includes Jolt
target_compile_definitions(other_physics
  PRIVATE
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
    # debug-family box3d carries the 'd' suffix (box3dd.lib vs box3d.lib)
    $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:ProfileD>>,box3dd,box3d>
)

other_install_module(other_physics "physics")
