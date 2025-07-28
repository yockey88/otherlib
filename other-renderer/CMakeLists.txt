add_library(other_renderer STATIC)
target_sources(other_renderer
    PRIVATE
      src/gpu_resource/renderer_resource.cpp
      src/gpu_resource/shader.cpp
      src/gpu_resource/texture.cpp
      src/gpu_resource/cube_map.cpp
      src/gpu_resource/gpu_buffer.cpp
      src/gpu_resource/mesh.cpp
      src/gpu_resource/framebuffer.cpp
      
      src/model/vertex.cpp
      src/model/vertex_buffer.cpp
      src/model/model_importer.cpp
      src/model/model.cpp
      
      src/renderer/window_manager.cpp
      
      src/renderer/rendering_api.cpp
      src/renderer/renderer_backend.cpp
      src/renderer/backends/opengl_api.cpp

      src/renderer/camera.cpp
      src/renderer/render_graph.cpp
      src/renderer/render_pipeline.cpp
      src/renderer/renderer.cpp

      src/renderer/draw_command.cpp
      
      src/renderer/ui/ui_helpers.cpp
)

target_include_directories(other_renderer 
  PUBLIC 
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      ${OTHER_EXTERN_DIR}/glad/include
      ${OTHER_EXTERN_DIR}/stb
      ${OTHER_EXTERN_DIR}/imgui
      ${OTHER_EXTERN_DIR}/sdl
      ${OTHER_EXTERN_DIR}/assimp
)
target_link_directories(other_renderer PUBLIC ${OTHER_LIBRARIES_PATH})

target_link_libraries(other_renderer PUBLIC imgui SDL3 glad other_core ${ASSIMP_NAME})