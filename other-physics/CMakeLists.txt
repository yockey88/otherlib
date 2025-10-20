add_library(other_physics STATIC)
target_sources(other_physics
  PRIVATE
    src/physics/physics_environment.cpp
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

target_link_libraries(other_physics PRIVATE ${PYTHON_LIB} other_core)

other_install_module(other_physics "physics")