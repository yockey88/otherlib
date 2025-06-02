add_library(other_renderer STATIC)
target_sources(other_renderer
    PRIVATE
      src/renderer/renderer_resource.cpp
      
      src/renderer/rendering_api.cpp
      src/renderer/backends/opengl_api.cpp
      
      src/renderer/window_manager.cpp
      src/renderer/renderer_backend.cpp
      
      src/renderer/render_graph.cpp
      src/renderer/renderer.cpp

      src/renderer/shader.cpp
      src/renderer/texture.cpp
      src/renderer/gpu_buffer.cpp
      src/renderer/mesh.cpp
      src/renderer/camera.cpp

      src/renderer/ui/ui_helpers.cpp

      src/model/vertex.cpp
      src/model/vertex_buffer.cpp
      src/model/model.cpp
)
if (MSVC)
else()
endif()

target_include_directories(other_renderer 
  PUBLIC 
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      ${OTHER_EXTERN_DIR}/stb
      ${OTHER_EXTERN_DIR}/imgui
      ${OTHER_EXTERN_DIR}/sdl
      ${OTHER_EXTERN_DIR}/glad/include
)
target_link_directories(other_renderer PUBLIC ${OTHER_LIBRARIES_PATH})
target_link_libraries(other_renderer PUBLIC imgui SDL3 glad other_core)