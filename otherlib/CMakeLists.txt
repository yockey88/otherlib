set(OTHER_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/src" "${OTHER_EXTERN_INCLUDES}")
if (MSVC)
    set(OTHER_INCLUDES ${OTHER_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/platform/windows")
else()
    set(OTHER_INCLUDES ${OTHER_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/platform/linux")
endif()

add_library(other_infrastructure STATIC)
target_sources(other_infrastructure
    PRIVATE
      src/core/command_line.cpp
      src/core/config_table.cpp
      src/core/defines.cpp
      src/core/logger.cpp
      src/core/arena.cpp
      src/core/registers.cpp
      src/core/ref.cpp
      src/core/ref_counted.cpp
      src/core/value.cpp

      src/math/random.cpp
      src/math/orthonormal_basis.cpp
      src/math/interval.cpp
      src/math/ray.cpp
      src/math/integrator.cpp

      src/plugin/library_handle.cpp
      src/plugin/plugin.cpp

      src/driver/driver.cpp
      src/driver/terminal_driver.cpp

      src/renderer/renderer_resource.cpp
      src/renderer/rendering_api.cpp
      src/renderer/backends/opengl_api.cpp
      src/renderer/renderer_backend.cpp
      src/renderer/renderer.cpp
      src/renderer/shader.cpp
      src/renderer/texture.cpp
      src/renderer/gpu_buffer.cpp
      src/renderer/mesh.cpp
      src/renderer/camera.cpp

      src/simulation/scene_object.cpp
      src/simulation/scene_tree.cpp
)
if (MSVC)
    target_sources(other_infrastructure
        PRIVATE
            platform/windows/windows_library_handle.cpp
            platform/windows/windows_plugin.cpp
    )
else()
### TODO: add linux support
endif()


target_include_directories(other_infrastructure PUBLIC ${OTHER_INCLUDES})
target_link_directories(other_infrastructure PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_infrastructure PUBLIC imgui SDL3 glad)

add_library(otherlib)
target_sources(otherlib PRIVATE src/other.cpp)
target_include_directories(otherlib PUBLIC ${OTHER_INCLUDES})
target_link_libraries(otherlib PUBLIC other_infrastructure)